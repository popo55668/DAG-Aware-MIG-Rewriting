"""
module pyabc_split

Executes python functions and their arguements as separate processes and returns their return values through pickling. This modules offers a single function:

Function: split_all(funcs)

The function returns a generator objects that allowes iteration over the results,

Arguments:

funcs: a list of tuples (f, args) where f is a python function and args is a collection of arguments for f.

Caveats:

1. Global variables in the parent process are not affected by the child processes.
2. The functions can only return simple types, see the pickle module for details

Usage:

Assume you would like to run the function f_1(1), f_2(1,2), f_3(1,2,3) in different processes. 

def f_1(i):
    return i+1
    
def f_2(i,j):
    return i*10+j+1
    
def f_3(i,j,k):
    return i*100+j*10+k+1

Construct a tuple of the function and arguments for each function

t_1 = (f_1, [1])
t_2 = (f_2, [1,2])
t_3 = (f_3, [1,2,3])

Create a list containing these tuples:

funcs = [t_1, t_2, t_3]

Use the function split_all() to run these functions in separate processes:

for res in split_all(funcs):
    print res
    
The output will be:

2
13
124

(The order may be different, except that in this case the processes are so fast that they terminate before the next one is created)

Alternatively, you may quite in the middle, say after the first process returns:

for res in split_all(funcs):
    print res
    break
    
This will kill all processes not yet finished.

To run ABC operations, that required saving the child process state and restoring it at the parent, use abc_split_all().

    import pyabc
    
    def abc_f(truth):
        import os
        print "pid=%d, abc_f(%s)"%(os.getpid(), truth)
        pyabc.run_command('read_truth %s'%truth)
        pyabc.run_command('strash')
        
    funcs = [
        defer(abc_f)("1000"),
        defer(abc_f)("0001")
    ]
    
    for _ in abc_split_all(funcs):
        pyabc.run_command('write_verilog /dev/stdout')

Author: Baruch Sterin <sterin@berkeley.edu>
"""

import os
import select
import fcntl
import errno
import sys
import cPickle as pickle
import signal
import cStringIO

import traceback

from contextlib import contextmanager

import pyabc

def _retry_select(rlist):
    while True:
        try:
            rrdy,_,_ = select.select(rlist,[],[])
            if rrdy:
                return rrdy
        except select.error as e:
            if e[0] == errno.EINTR:
                continue
            raise

class _splitter(object):
    
    def __init__(self):
        self.pids = []
        self.fds = {}
        self.buffers = {}
        self.results = {}
    
    def is_done(self):
        return len(self.fds) == 0
    
    def _kill(self, pids):

        # close pipes and kill child processes
        for pid in pids:

            if pid == -1:
                continue

            i, fd = self.fds[pid]
            
            del self.buffers[fd]
            del self.fds[pid]

            self.pids[i] = -1
            self.results[pid] = None
            
            os.close(fd)
            
            try:
                os.kill( pid, signal.SIGINT)
            except Exception as e:
                print >>sys.stderr, 'exception while trying to kill pid=%d: '%pid, e
                raise

        # wait for termination and update result
        for pid in pids:
            os.waitpid( pid, 0 )

    def kill(self, ids):
        
        self._kill( [ self.pids[i] for i in ids ] )
            
    def cleanup(self):
        self._kill( self.fds.keys() )

    def child( self, fdw, f):

        # call function
        try:
            res = f()
        except:
            traceback.print_exc()
            raise
        
        # write return value into pipe
        with os.fdopen( fdw, "w" ) as fout:
            pickle.dump(res, fout)
            
        return 0

    def _fork_one(self, f):
        
        # create a pipe to communicate with the child process
        pr,pw = os.pipe()
        
        # set pr to be non-blocking
        fcntl.fcntl(pr, fcntl.F_SETFL, os.O_NONBLOCK)
        
        parentpid = os.getpid()
        rc = 1
        
        try:

            # create child process
            pid = os.fork()
    
            if pid == 0:
                # child process:
                os.close(pr)
                pyabc.close_on_fork(pw)
                
                rc = self.child( pw, f)
                os._exit(rc)
            else:
                # parent process:
                os.close(pw)
                return (pid, pr)
                
        finally:
            if os.getpid() != parentpid:
                os._exit(rc)

    def fork_one(self, func):
        pid, fd = self._fork_one(func)
        i = len(self.pids)
        self.pids.append(pid)
        self.fds[pid] = (i, fd)
        self.buffers[fd] = cStringIO.StringIO()
        return i
    
    def fork_all(self, funcs):
        return [ self.fork_one(f) for f in funcs ]

    def communicate(self):
        
        rlist = [ fd for _, (_,fd) in self.fds.iteritems() ]
        rlist.append(pyabc.wait_fd)

        stop = False
        
        while not stop:

            rrdy = _retry_select( rlist )
            
            for fd in rrdy:
                
                if fd == pyabc.wait_fd:
                    stop = True
                    continue
                    
                self.buffers[fd].write( os.read(fd, 16384) )
    
    def get_next_result(self):

        # read from the pipes as needed, while waiting for the next child process to terminate
        self.communicate()
    
        # wait for the next child process to terminate
        pid, rc = os.wait()
        assert pid in self.fds
        
        # retrieve the pipe file descriptor
        i, fd = self.fds[pid]
        del self.fds[pid]

        # remove the pid
        self.pids[i] = -1

        # retrieve the buffer
        buffer = self.buffers[fd]
        del self.buffers[fd]

        # fill the buffer
        while True:
            s = os.read(fd, 16384)
            if not s:
                break
            buffer.write(s)
            
        os.close(fd)

        try:
            return (i, pickle.loads(buffer.getvalue()))
        except EOFError, pickle.UnpicklingError:
            return (i, None)
            
    def __iter__(self):
        def iterator():
            while not self.is_done():
                yield self.get_next_result()
        return iterator()

@contextmanager
def make_splitter():
    # ensure cleanup of child processes
    s = _splitter()
    try:
        yield s
    finally:
        s.cleanup()
        
def split_all_full(funcs):
    # provide an iterator for child process result
    with make_splitter() as s:
        
        s.fork_all(funcs)
        
        for res in s:
            yield res

def defer(f):
    return lambda *args, **kwargs: lambda : f(*args,**kwargs)

def split_all(funcs):
    for _, res in split_all_full( ( defer(f)(*args) for f,args in funcs ) ):
        yield res

import tempfile

@contextmanager
def temp_file_names(suffixes):
    names = []
    try:
        for suffix in suffixes:
            with tempfile.NamedTemporaryFile(delete=False, suffix=suffix) as file:
                names.append( file.name )
        yield names
    finally:
        for name in names:
            os.unlink(name)

class abc_state(object):
    def __init__(self):
        with tempfile.NamedTemporaryFile(delete=False, suffix='.aig') as file:
            self.aig = file.name
        with tempfile.NamedTemporaryFile(delete=False, suffix='.log') as file:
            self.log = file.name
        pyabc.run_command(r'write_status %s'%self.log)
        pyabc.run_command(r'write_aiger %s'%self.aig)
    
    def __del__(self):
        os.unlink( self.aig )
        os.unlink( self.log )

    def restore(self):
        pyabc.run_command(r'read_aiger %s'%self.aig)
        pyabc.run_command(r'read_status %s'%self.log)

def abc_split_all(funcs):
    import pyabc
    
    def child(f, aig, log):
        res = f()
        pyabc.run_command(r'write_status %s'%log)
        pyabc.run_command(r'write_aiger %s'%aig)
        return res
        
    def parent(res, aig, log):
        pyabc.run_command(r'read_aiger %s'%aig)
        pyabc.run_command(r'read_status %s'%log)
        return res
        
    with temp_file_names( ['.aig','.log']*len(funcs) ) as tmp:

        funcs = [ defer(child)(f, tmp[2*i],tmp[2*i+1]) for i,f in enumerate(funcs) ]
        
        for i, res in split_all_full(funcs):
            yield i, parent(res, tmp[2*i],tmp[2*i+1])

if __name__ == "__main__":
    
    # define some functions to run
    
    def f_1(i):
        return i+1
        
    def f_2(i,j):
        return i*10+j+1
        
    def f_3(i,j,k):
        return i*100+j*10+k+1
    
    # Construct a tuple of the function and arguments for each function
    
    t_1 = (f_1, [1])
    t_2 = (f_2, [1,2])
    t_3 = (f_3, [1,2,3])

    # Create a list containing these tuples:

    funcs = [t_1, t_2, t_3]

    # Use the function split_all() to run these functions in separate processes:

    for res in split_all(funcs):
        print res

    # Alternatively, quit after the first process returns:
    
    for res in split_all(funcs):
        print res
        break
        
    # For operations with ABC that save and restore status
    
    import pyabc
    
    def abc_f(truth):
        import os
        print "pid=%d, abc_f(%s)"%(os.getpid(), truth)
        pyabc.run_command('read_truth %s'%truth)
        pyabc.run_command('strash')
        return 100
        
    funcs = [
        defer(abc_f)("1000"),
        defer(abc_f)("0001")
    ]
    
    best = None
    
    for i, res in abc_split_all(funcs):
        print i, res
        if best is None:\
            # save state
            best = abc_state()
        pyabc.run_command('write_verilog /dev/stdout')
    
    # if there is a saved state, restore it
    if best is not None:
        best.restore()
        pyabc.run_command('write_verilog /dev/stdout')

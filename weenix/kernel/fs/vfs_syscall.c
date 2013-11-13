/*
 *  FILE: vfs_syscall.c
 *  AUTH: mcc | jal
 *  DESC:
 *  DATE: Wed Apr  8 02:46:19 1998
 *  $Id: vfs_syscall.c,v 1.1 2012/10/10 20:06:46 william Exp $
 */

#include "kernel.h"
#include "errno.h"
#include "globals.h"
#include "fs/vfs.h"
#include "fs/file.h"
#include "fs/vnode.h"
#include "fs/vfs_syscall.h"
#include "fs/open.h"
#include "fs/fcntl.h"
#include "fs/lseek.h"
#include "mm/kmalloc.h"
#include "util/string.h"
#include "util/printf.h"
#include "fs/stat.h"
#include "util/debug.h"

/* To read a file:
 *      o fget(fd)
 *      o call its virtual read f_op
 *      o update f_pos
 *      o fput() it
 *      o return the number of bytes read, or an error
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not a valid file descriptor or is not open for reading.
 *      o EISDIR
 *        fd refers to a directory.
 *
 * In all cases, be sure you do not leak file refcounts by returning before
 * you fput() a file that you fget()'ed.
 */
int
do_read(int fd, void *buf, size_t nbytes)
{
      /*NOT_YET_IMPLEMENTED("VFS: do_read");*/
      file_t *fle;

      fle=fget(fd);
      if(fle==NULL)
      {
          /*DBG(DBG_INIT,"file not found");*/
          return -EBADF;
      }

      if(_S_TYPE(fle->f_vnode->vn_mode)==S_IFDIR)
      {
          dbg(DBG_INIT,"file is a directory");
          fput(fle);
          return -EISDIR;
      }
      if((fle->f_mode&FMODE_READ)==0)
      {
          dbg(DBG_INIT,"file is not in read mode");
          fput(fle);
          return -EBADF;
      }
      int amt_read = fle->f_vnode->vn_ops->read(fle->f_vnode,fle->f_pos, buf, nbytes);
      fle->f_pos=fle->f_pos + amt_read;
      fput(fle);
      return amt_read;
}

/* Very similar to do_read.  Check f_mode to be sure the file is writable.  If
 * f_mode & FMODE_APPEND, do_lseek() to the end of the file, call the write
 * f_op, and fput the file.  As always, be mindful of refcount leaks.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not a valid file descriptor or is not open for writing.
 */
int
do_write(int fd, const void *buf, size_t nbytes)
{
      /*NOT_YET_IMPLEMENTED("VFS: do_write");*/
      file_t *fle;

      fle=fget(fd);
      if(fle==NULL)
      {
          /*DBG(DBG_INIT,"file not found");*/
          return -EBADF;
      }

      if(_S_TYPE(fle->f_vnode->vn_mode)==S_IFDIR)
      {
          dbg(DBG_INIT,"file is a directory");
          fput(fle);
          return -EISDIR;
      }
      if((fle->f_mode&FMODE_WRITE) == 0)
      {

          dbg(DBG_INIT,"file is not in write mode");
          fput(fle);
          return -EBADF;
      }

      if(fle->f_mode==FMODE_APPEND)
      {
         do_lseek(fd, 0, SEEK_END);
      }

      int amt_write = fle->f_vnode->vn_ops->write(fle->f_vnode,fle->f_pos,buf,nbytes);
      fle->f_pos=fle->f_pos + amt_write;
      fput(fle);
      if (amt_write){
          KASSERT((S_ISCHR(fle->f_vnode->vn_mode)) ||
                  (S_ISBLK(fle->f_vnode->vn_mode)) ||
                  ((S_ISREG(fle->f_vnode->vn_mode)) && (fle->f_pos <= fle->f_vnode->vn_len)));
      }
      return amt_write;
}

/*
 * Zero curproc->p_files[fd], and fput() the file. Return 0 on success
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd isn't a valid open file descriptor.
 */
int
do_close(int fd)
{
       /*NOT_YET_IMPLEMENTED("VFS: do_close");*/
       file_t *fil=fget(fd);
        if(fil==NULL)
                return -EBADF;
        curproc->p_files[fd]=NULL;
        fput(fil);
        return 0;
}

/* To dup a file:
 *      o fget(fd) to up fd's refcount
 *      o get_empty_fd()
 *      o point the new fd to the same file_t* as the given fd
 *      o return the new file descriptor
 *
 * Don't fput() the fd unless something goes wrong.  Since we are creating
 * another reference to the file_t*, we want to up the refcount.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd isn't an open file descriptor.
 *      o EMFILE
 *        The process already has the maximum number of file descriptors open
 *        and tried to open a new one.
 */
int
do_dup(int fd)
{
      /*NOT_YET_IMPLEMENTED("VFS: do_dup");*/
      file_t *orig_fil=fget(fd);
      if(fd==NULL){
        return -EBADF;
      }
      
      int fd_new=get_empty_fd(curproc);
      if(fd_new==-EMFILE)
      {
           fput(orig_fil);
           return -EMFILE;
      }
      curproc->p_files[fd_new]=orig_fil;
      return fd_new;
}

/* Same as do_dup, but insted of using get_empty_fd() to get the new fd,
 * they give it to us in 'nfd'.  If nfd is in use (and not the same as ofd)
 * do_close() it first.  Then return the new file descriptor.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        ofd isn't an open file descriptor, or nfd is out of the allowed
 *        range for file descriptors.
 */
int
do_dup2(int ofd, int nfd)
{
        /*NOT_YET_IMPLEMENTED("VFS: do_dup2");*/
        file_t *fil=fget(ofd);
        if(fil==NULL)
                return -EBADF;
        if(nfd>NFILES)
        {
                fput(fil);
                return -EBADF;
        }
        if(curproc->p_files[nfd]!=NULL && curproc->p_files[nfd]!=fil)
                do_close(nfd);
        curproc->p_files[nfd]=fil;
        return nfd;
}

/*
 * This routine creates a special file of the type specified by 'mode' at
 * the location specified by 'path'. 'mode' should be one of S_IFCHR or
 * S_IFBLK (you might note that mknod(2) normally allows one to create
 * regular files as well-- for simplicity this is not the case in Weenix).
 * 'devid', as you might expect, is the device identifier of the device
 * that the new special file should represent.
 *
 * You might use a combination of dir_namev, lookup, and the fs-specific
 * mknod (that is, the containing directory's 'mknod' vnode operation).
 * Return the result of the fs-specific mknod, or an error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        mode requested creation of something other than a device special
 *        file.
 *      o EEXIST
 *        path already exists.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_mknod(const char *path, int mode, unsigned devid)
{	
        /*NOT_YET_IMPLEMENTED("VFS: do_mknod");*/
        const char *name;
        size_t namelen = 0;
        if(!S_ISBLK(mode)&& !S_ISCHR(mode)){
                return -EINVAL;
        }
        vnode_t *res_node,*result;
        if(strlen(path)>MAXPATHLEN)
                return -ENAMETOOLONG;
        
        int retval=dir_namev(path, &namelen, &name, NULL, &res_node);
        if(retval==-ENOTDIR)
                return -ENOTDIR;
                dbg(DBG_INIT,"MKNOD before lookup %s\n",name);
        retval= lookup(res_node, name, namelen, &result);
                dbg(DBG_INIT,"MKNOD after lookup %d\n",retval);
        if(retval>0)
        {
             vput(result);
             return -EEXIST;
        }
        /*if(S_IFBLK(res_node->vn_mode)|| S_IFCHR(res_node->vn_mode))*/
        return res_node->vn_ops->mknod(res_node, name, namelen, mode, devid);
}

/* Use dir_namev() to find the vnode of the dir we want to make the new
 * directory in.  Then use lookup() to make sure it doesn't already exist.
 * Finally call the dir's mkdir vn_ops. Return what it returns.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EEXIST
 *        path already exists.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_mkdir(const char *path)
{
        /*NOT_YET_IMPLEMENTED("VFS: do_mkdir");*/
	const char *name;
        size_t namelen;
        vnode_t *res_node;

        if(strlen(path)>MAXPATHLEN)
                return -ENAMETOOLONG;
        dbg(DBG_INIT,"MKDIRxxxx before lookup %s\n",path);
        int retval=dir_namev(path, &namelen, &name, NULL, &res_node);
        dbg(DBG_INIT,"MKDIRxxxx after lookup\n");
       /* dbg(DBG_INIT,"MKDIR after lookup %d,%f\n",retval,res_node->vn_vno); */
        if(retval==-ENOTDIR)
                return -ENOTDIR;

        retval=lookup(res_node, name, namelen, &res_node);
        if(retval>0)
        {
            dbg(DBG_INIT,"return value from look > 0\n");        
            vput(res_node);
            return -EEXIST;
        }
	dbg(DBG_INIT,"return value from look <= 0\n");   
	dbg(DBG_INIT,"CREATING MKNODDDDDDDDDDDDDDDDDDDDDD %d,%s,%d\n",res_node->vn_vno,name,namelen);   
        int status= res_node->vn_ops->mkdir(res_node, name, namelen);
        dbg(DBG_INIT,"end of mkdir %d\n",status); 
}

/* Use dir_namev() to find the vnode of the directory containing the dir to be
 * removed. Then call the containing dir's rmdir v_op.  The rmdir v_op will
 * return an error if the dir to be removed does not exist or is not empty, so
 * you don't need to worry about that here. Return the value of the v_op,
 * or an error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        path has "." as its final component.
 *      o ENOTEMPTY
 *        path has ".." as its final component.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int do_rmdir(const char *path)
{
   NOT_YET_IMPLEMENTED("VFS: do_rmdir");
   int len1=0,len2=0,len3=0;
   size_t path_len;
   const char *path_name;
   vnode_t *path_vnode;
   int ret_val,ret_code;

   if(strlen(path)<1)
   { 
      return -EINVAL;
   }

   ret_val=dir_namev(path, &path_len, &path_name, NULL, &path_vnode);

   if(ret_val < 0){
       return ret_val;
   }
   
   len1=strlen(path_name);
   len2=len1-1;
   len3=len1-2;

   if(path_name[len2]=='.')
   {
	if(path[len3]=='.')
	   return -ENOTEMPTY;
	else
	   return -EINVAL;
   }
   KASSERT(NULL != path_vnode->vn_ops->rmdir);
   dbg(DBG_INIT,"(GRADING2 3.d)  Directory's vnode is not null\n");
   
   ret_code=path_vnode->vn_ops->rmdir(path_vnode,path_name,path_len);

   return ret_code;
}

/*
 * Same as do_rmdir, but for files.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EISDIR
 *        path refers to a directory.
 *      o ENOENT
 *        A component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_unlink(const char *path)
{
        /*OT_YET_IMPLEMENTED("VFS: do_unlink");*/
        dbg(DBG_INIT,"DO_UNLINK %s\n",path);
        const char *name;
        size_t namelen = 0;
        vnode_t *res_node,*result;
        if(strlen(path)>MAXPATHLEN)
                return -ENAMETOOLONG;
        int retval=dir_namev(path, &namelen, &name,NULL, &res_node);
        dbg(DBG_INIT,"DO_UNLINK: Name to be unlinked %s\n", name);
        if(retval < 0){
            return retval;
        }

        retval=lookup(res_node, name,namelen, &res_node);
        if(retval < 0){
            return retval;
        }
        /*
        if(retval==-ENOTDIR)
                return -ENOTDIR;
        if(retval==-ENOENT)
                return -ENOENT;
        */

        if(S_ISDIR(res_node->vn_mode))
        {
                vput(res_node);
                return -EISDIR;
        }
        return res_node->vn_ops->unlink(res_node, name, namelen);
}

/* To link:
 *      o open_namev(from)
 *      o dir_namev(to)
 *      o call the destination dir's (to) link vn_ops.
 *      o return the result of link, or an error
 *
 * Remember to vput the vnodes returned from open_namev and dir_namev.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EEXIST
 *        to already exists.
 *      o ENOENT
 *        A directory component in from or to does not exist.
 *      o ENOTDIR
 *        A component used as a directory in from or to is not, in fact, a
 *        directory.
 *      o ENAMETOOLONG
 *        A component of from or to was too long.
 */
int
do_link(const char *from, const char *to)
{
        /* NOT_YET_IMPLEMENTED("VFS: do_link");*/
        const char *name ;
        size_t namelen;
        vnode_t *res_node_source, *res_node_dest,*result;
        if(strlen(from)>MAXPATHLEN)
                return -ENAMETOOLONG;
        if(strlen(to)>MAXPATHLEN)
                return -ENAMETOOLONG;

        int retval=open_namev(from,0, &res_node_source,NULL);
        if(retval < 0){
            /*vput(res_node_source);*/
            return retval;
        }

        retval=dir_namev(to, &namelen, &name,NULL, &res_node_dest);
        if(retval < 0){
            /*vput(res_node_dest);
            vput(res_node_source);*/
            return retval;
        }
        /*
        if(retval=-ENOTDIR)
                return -ENOTDIR;
        if(retval=-ENOENT)
                return -ENOENT;
        */

        retval=lookup(res_node_dest, name,namelen, &result);
        if(retval>0)
               { vput(result);
                 return -EEXIST;
                }
        retval=res_node_dest->vn_ops->link(res_node_dest,res_node_source, name,namelen);

        vput(res_node_dest);
        vput(res_node_source);
        return retval;
}

/*      o link newname to oldname
 *      o unlink oldname
 *      o return the value of unlink, or an error
 *
 * Note that this does not provide the same behavior as the
 * Linux system call (if unlink fails then two links to the
 * file could exist).
 */
int
do_rename(const char *oldname, const char *newname)
{
        /*_YET_IMPLEMENTED("VFS: do_rename");*/
        int retval=do_link(newname,oldname);
        if(retval<=0)
                return retval;
        else
                return do_unlink(oldname);
}

/* Make the named directory the current process's cwd (current working
 * directory).  Don't forget to down the refcount to the old cwd (vput()) and
 * up the refcount to the new cwd (open_namev() or vget()). Return 0 on
 * success.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o ENOENT
 *        path does not exist.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 *      o ENOTDIR
 *        A component of path is not a directory.
 */
int
do_chdir(const char *path)
{
        /*  NOT_YET_IMPLEMENTED("VFS: do_chdir");*/
	vnode_t *res_node;
        if(strlen(path)>MAXPATHLEN)
                return -ENAMETOOLONG;
        int retval=open_namev(path,0, &res_node,NULL);
        if(retval==-ENOTDIR)
                return -ENOTDIR;
        if(retval==-ENOENT)
                return -ENOENT;
        vput(curproc->p_cwd);
        curproc->p_cwd=res_node;
        return 0;
}

/* Call the readdir f_op on the given fd, filling in the given dirent_t*.
 * If the readdir f_op is successful, it will return a positive value which
 * is the number of bytes copied to the dirent_t.  You need to increment the
 * file_t's f_pos by this amount.  As always, be aware of refcounts, check
 * the return value of the fget and the virtual function, and be sure the
 * virtual function exists (is not null) before calling it.
 *
 * Return either 0 or sizeof(dirent_t), or -errno.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        Invalid file descriptor fd.
 *      o ENOTDIR
 *        File descriptor does not refer to a directory.
 */
int
do_getdent(int fd, struct dirent *dirp)
{
   /*NOT_YET_IMPLEMENTED("VFS: do_getdent DONE");*/
   dbg(DBG_INIT,"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX getdent 1\n");
   int to_add;
   file_t *file_fd;
   
   file_fd=fget(fd);
   if(file_fd ==NULL)
   {
      return -EBADF;
   }
   if(!S_ISDIR(file_fd->f_vnode->vn_mode))
   {
      fput(file_fd);
      return -ENOTDIR;
   }
   if(!file_fd->f_vnode->vn_ops->readdir){
      fput(file_fd);
      return 0;
   }
   to_add = file_fd->f_vnode->vn_ops->readdir(file_fd->f_vnode,file_fd->f_pos,dirp);
   dbg(DBG_INIT,"TO_ADD = %d\n", to_add);
   if(to_add==0){
       return 0;
   }
   if(to_add>0)
      file_fd->f_pos = file_fd->f_pos + to_add;
    fput(file_fd);
   dbg(DBG_INIT,"Size = %d\n", sizeof(*dirp));
  
    return sizeof(*dirp);
}

/*
 * Modify f_pos according to offset and whence.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not an open file descriptor.
 *      o EINVAL
 *        whence is not one of SEEK_SET, SEEK_CUR, SEEK_END; or the resulting
 *        file offset would be negative.
 */
int
do_lseek(int fd, int offset, int whence)
{
        /*NOT_YET_IMPLEMENTED("VFS: do_lseek DONE");*/
        /* Get the file */
        file_t *fle;
        int fpos;
        fle=fget(fd);
        if(fle==NULL)
        {
            dbg(DBG_PRINT,"no files available");
            return -EBADF;
        }

        /*SEEK_SET: Relative to beginining of file*/
        /*SEEK_CUR: Relative to current position of file*/
        /*SEEK_END: Relative to end of the file. the offset must be negative*/
        if(whence==SEEK_SET)
        {
            fpos=offset;
        }
        else if(whence == SEEK_CUR)
        {
            fpos=fle->f_pos+offset;

        }
        else if(whence == SEEK_END)
        {
            /*TODO Check or not*/
            fpos=fle->f_vnode->vn_len+offset;
        }
        else{
            fput(fle);
            return -EINVAL;
        }

        if(fpos<0){
            fput(fle);
            return -EINVAL;
        }
        fle->f_pos = fpos;
        fput(fle);
        return 0; 
}

/*
 * Find the vnode associated with the path, and call the stat() vnode operation.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o ENOENT
 *        A component of path does not exist.
 *      o ENOTDIR
 *        A component of the path prefix of path is not a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int do_stat(const char *path, struct stat *buf)
{
  /*NOT_YET_IMPLEMENTED("VFS: do_stat DONE");*/
     dbg(DBG_INIT,"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX DOSTAT 1\n");
  int ret_val,ret_code;
  vnode_t *get_vnode;

  ret_val=open_namev(path, O_RDONLY, &get_vnode, NULL);

  KASSERT(get_vnode->vn_ops->stat);
  dbg(DBG_INIT,"(GRADING2 3.f) vnode exists\n");

  if(ret_val < 0){
      return ret_val;
  }
  /*
  if(ret_val == -ENOENT)
     return ret_code;
  if(ret_val == -ENOTDIR)
     return ret_code;
  if(ret_val == -ENAMETOOLONG)
     return ret_code;
  */
   
  ret_code=get_vnode->vn_ops->stat(get_vnode,buf); 
  return ret_code;
}

#ifdef __MOUNTING__
/*
 * Implementing this function is not required and strongly discouraged unless
 * you are absolutely sure your Weenix is perfect.
 *
 * This is the syscall entry point into vfs for mounting. You will need to
 * create the fs_t struct and populate its fs_dev and fs_type fields before
 * calling vfs's mountfunc(). mountfunc() will use the fields you populated
 * in order to determine which underlying filesystem's mount function should
 * be run, then it will finish setting up the fs_t struct. At this point you
 * have a fully functioning file system, however it is not mounted on the
 * virtual file system, you will need to call vfs_mount to do this.
 *
 * There are lots of things which can go wrong here. Make sure you have good
 * error handling. Remember the fs_dev and fs_type buffers have limited size
 * so you should not write arbitrary length strings to them.
 */
int
do_mount(const char *source, const char *target, const char *type)
{
        NOT_YET_IMPLEMENTED("MOUNTING: do_mount");
        return -EINVAL;
}

/*
 * Implementing this function is not required and strongly discouraged unless
 * you are absolutley sure your Weenix is perfect.
 *
 * This function delegates all of the real work to vfs_umount. You should not worry
 * about freeing the fs_t struct here, that is done in vfs_umount. All this function
 * does is figure out which file system to pass to vfs_umount and do good error
 * checking.
 */
int
do_umount(const char *target)
{
        NOT_YET_IMPLEMENTED("MOUNTING: do_umount");
        return -EINVAL;
}
#endif

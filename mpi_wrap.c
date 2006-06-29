/*****************************************************************************\
 *  $id: mpi_wrap.c,v 1.7 2006/01/09 16:26:39 auselton Exp $
 *****************************************************************************
 *  Copyright (C) 2001-2002 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Andrew Uselton <uselton2@llnl.gov>
 *  UCRL-CODE-2006-xxx.
 *  
 *  This file is part of Mib, an MPI-based parallel I/O benchamrk
 *  For details, see <http://www.llnl.gov/linux/mib/>.
 *  
 *  Mib is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *  
 *  Mib is distributed in the hope that it will be useful, but WITHOUT 
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
 *  for more details.
 *  
 *  You should have received a copy of the GNU General Public License along
 *  with Mib; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
\*****************************************************************************/

#include <stdio.h>
#include <string.h>     /* for strncpy */
#include <dlfcn.h>      /* dynamic loader interface */
#include "config.h"
#include "mpi_wrap.h"
#include "mib.h"        /* for conditional_report */
#include "miberr.h"
#include "options.h"
#include "slurm.h"

/*
 * Some of the error handling might be improved if some MPI calls don't
 * have to FAIL and MPI_Abort but can defer their complaint to the 
 * next barrier.
 * The USE_MPI guard might be accompanied by an error in many cases, since
 * there is no good way of emulating the behavior without. 
 */

/* 
 *   the code is organized so that it will (at least try to) run even
 *   without MPI.
 */
int use_mpi = NO_MPI;
/*
 *   Three libraries must be loaded if MPI is going to be used.  "use_mpi"
 * is only set "true" if all three are sucessfully loaded.
 */
/*
 * The old handles
void *mpi_handle;
void *mpio_handle;
void *elan_handle;
*/
void **lib_handles;
char *libraries[] = LIBRARIES;

int (*pMPI_Abort)(MPI_Comm comm, int errorcode);
int (*pMPI_Allreduce)(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, 
		       MPI_Op op, MPI_Comm comm);
int (*pMPI_Barrier)(MPI_Comm comm);
int (*pMPI_Bcast)(void *buffer, int count, MPI_Datatype datatype, 
	  int root, MPI_Comm comm);
int (*pMPI_Comm_create)(MPI_Comm comm, MPI_Group group,MPI_Comm *newcomm);
int (*pMPI_Comm_free)(MPI_Comm *comm);
int (*pMPI_Comm_group)(MPI_Comm comm, MPI_Group *group);
int (*pMPI_Comm_rank)(MPI_Comm comm, int *rankp);
int (*pMPI_Comm_size)(MPI_Comm comm, int *sizep);
int (*pMPI_Comm_split)(MPI_Comm comm, int color, int key, MPI_Comm *newcomm);
int (*pMPI_Errhandler_set)(MPI_Comm comm, MPI_Errhandler errhandler);
int (*pMPI_Finalize)(void);
int (*pMPI_Gather)(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, 
		    int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int (*pMPI_Group_free)(MPI_Group *group);
int (*pMPI_Group_range_incl)(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup);
int (*pMPI_Init)(int *argcp, char ***argvp);
int (*pMPI_Recv)(void *buf, int count, MPI_Datatype datatype,  
	 int source, int tag, MPI_Comm comm, MPI_Status *status);
int (*pMPI_Reduce)(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, 
		    MPI_Op op, int root, MPI_Comm comm);
int (*pMPI_Send)(void *buffer, int count, MPI_Datatype datatype, 
	      int dest, int tag, MPI_Comm comm);
double (*pMPI_Wtime)();

extern Options *opts;
extern SLURM *slurm;

void
mpi_abort(MPI_Comm comm, int errorcode)
{
  if( USE_MPI )
    {
      (*pMPI_Abort)(comm, errorcode);
    }
  else
    {
      exit(errorcode);
    }
}

void
mpi_allreduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int rc = 0;
  int size = 1;

  if(USE_MPI)
    {
      if((rc = (*pMPI_Allreduce)(sendbuf, recvbuf, count, datatype, op, comm)) != MPI_SUCCESS)
	FAIL();
    }
  else
    {
      switch (datatype)
	{
	case MPI_INT :
	  size = 4;
	  break;
	case MPI_DOUBLE : 
	  size = 8;
	  break;
	case MPI_CHAR :
	default :
	  size = 1;
	  break;
	}
      size *= count;
      memcpy(recvbuf, sendbuf, size);
    }
}

void
mpi_barrier(MPI_Comm comm)
{
  int rc = 0;
  
  /*
   *   This code no longer checks for delayed error reports.  I'll
   * want to reintroduce that once I get error handling on a more
   * solid footing.
   */
  if(USE_MPI)
    if((rc = (*pMPI_Barrier)(comm)) != MPI_SUCCESS)
      FAIL();
}

void
mpi_bcast(void *buffer, int count, MPI_Datatype datatype, 
	 int root, MPI_Comm comm)
{
  int rc = 0;

  if(USE_MPI)
    if((rc = (*pMPI_Bcast)(buffer, count, datatype, root, comm)) != MPI_SUCCESS)
      FAIL();
}

void
mpi_comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm)
{
  int rc = 0;

  if(USE_MPI)
    if((rc = (*pMPI_Comm_create)(comm, group, newcomm)) != MPI_SUCCESS)
      FAIL();
}

void
mpi_comm_free(MPI_Comm *comm)
{
  int rc = 0;

  if(USE_MPI)
    if((rc = (*pMPI_Comm_free)(comm)) != MPI_SUCCESS)
      FAIL();
}

void
mpi_comm_group(MPI_Comm comm, MPI_Group *group)
{
  int rc = 0;

  if(USE_MPI)
    if((rc = (*pMPI_Comm_group)(comm, group)) != MPI_SUCCESS)
      FAIL();
}

void
mpi_comm_rank(MPI_Comm comm, int *rankp)
{
  int rc = 0;

  if(USE_MPI)
    {
      if((rc = (*pMPI_Comm_rank)(comm, rankp)) != MPI_SUCCESS)
	FAIL();
    }
}

void
mpi_comm_size(MPI_Comm comm, int *sizep)
{
  int rc = 0;

  if(USE_MPI)
    {
      if((rc = (*pMPI_Comm_size)(comm, sizep)) != MPI_SUCCESS)
	FAIL();
    }
}

void
mpi_comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm)
{
  int rc = 0;

  if(USE_MPI)
    if((rc = (*pMPI_Comm_split)(comm, color, key, newcomm)) != MPI_SUCCESS)
      FAIL();
}

void 
mpi_errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler)
{
  int rc = 0;

  if(USE_MPI)
    {
      if((rc = (*pMPI_Errhandler_set)(comm, errhandler)) != MPI_SUCCESS)
	FAIL();
    }
}

void
mpi_finalize(void)
{
  int rc = 0;
  int lib;

  if(USE_MPI)
    {
      if((rc = (*pMPI_Finalize)()) != MPI_SUCCESS)
	FAIL();
      /*
       * The old way
      dlclose(mpi_handle);
      dlclose(mpio_handle);
      dlclose(elan_handle);
      */    
      /*
       * There is uglyness in Elan-land.  The elan3-based libraries unload just
       * fine, but the elan4 libraries do not.  If you just let process termination
       * sort things out it works fine, too.  This is the end of the program so
       * it shouldn't make any difference that we don't close libelan ourselves.
      */
      for(lib = 0; lib < NUM_LIBRARIES; lib++)
	{
	  if( ! strncmp(libraries[lib], "libelan.so", 10) )
	     dlclose(lib_handles[lib]);
	}
    }
}

void
mpi_gather(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int rc = 0;

  if(USE_MPI)
    if((rc = (*pMPI_Gather)(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm)) != MPI_SUCCESS)
      FAIL();
}

void
mpi_group_free(MPI_Group *group)
{
  int rc = 0;

  if(USE_MPI)
    if((rc = (*pMPI_Group_free)(group)) != MPI_SUCCESS)
      FAIL();
}

void
mpi_group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup)
{
  int rc = 0;

  if(USE_MPI)
    if((rc = (*pMPI_Group_range_incl)(group, n, ranges, newgroup)) != MPI_SUCCESS)
      FAIL();
}

void
mpi_init(int *argcp, char ***argvp)
{
  /*
   * the old way
  char mpi[] = "libmpi.so";
  char mpio[] = "libmpio.so";
  char elan[] = "libelan.so";
  */
  int rc = 0;
  int flag = RTLD_LAZY | RTLD_GLOBAL;
  char *dlerr;
  int lib;

  if( use_mpi == FORCE_NO_MPI )
    {
      conditional_report(SHOW_ENVIRONMENT, "Command line forbade the use of MPI\n");
      return;
    }
  if( ! slurm->use_SLURM )
    {
      conditional_report(SHOW_ENVIRONMENT, "No SLURM, no MPI\n");
      return;
    }
  dlerror(); /* clear any preexisting error */
  /*
   * The old way
  if( (elan_handle = dlopen(elan, flag)) == NULL)
    {
      if( (dlerr = dlerror()) != NULL ) 
	conditional_report(SHOW_ENVIRONMENT, "No MPI - ELAN lib (%s) not loaded: %s\n", elan, dlerr);
      else
	conditional_report(SHOW_ENVIRONMENT, "No MPI - ELAN lib (%s) not loaded: no dlerror string, though\n", elan);
      return;
    }
  if( (mpi_handle = dlopen(mpi, flag)) == NULL)
    {
      if( (dlerr = dlerror()) != NULL ) 
	conditional_report(SHOW_ENVIRONMENT, "No MPI - MPI lib (%s) not loaded: %s\n", mpi, dlerr);
      else
	conditional_report(SHOW_ENVIRONMENT, "No MPI - MPI lib (%s) not loaded: no dlerror string, though\n", mpi);
      return;
    }
  if( (mpio_handle = dlopen(mpio, flag)) == NULL)
    {
      if( (dlerr = dlerror()) != NULL ) 
	conditional_report(SHOW_ENVIRONMENT, "No MPI - MPIO lib (%s) not loaded: %s\n", mpio, dlerr);
      else
	conditional_report(SHOW_ENVIRONMENT, "No MPI - MPIO lib (%s) not loaded: no dlerror string, though\n", mpio);
      return;
    }
  */
  lib_handles = (void **)Malloc(sizeof(void*)*NUM_LIBRARIES);
  for (lib = 0; lib < NUM_LIBRARIES; lib++)
    {
      if( (lib_handles[lib] = dlopen(libraries[lib], flag)) == NULL)
	{
	  if( (dlerr = dlerror()) != NULL ) 
	    conditional_report(SHOW_ENVIRONMENT, "No MPI - library %s not loaded: %s\n", libraries[lib], dlerr);
	  else
	    conditional_report(SHOW_ENVIRONMENT, "No MPI - library %s not loaded: no dlerror string, though\n", libraries[lib]);
	  return;
	}     
      lib++;
    }
  pMPI_Abort = dlsym(lib_handles[MPI_HANDLE], "MPI_Abort");
  pMPI_Allreduce = dlsym(lib_handles[MPI_HANDLE], "MPI_Allreduce");
  pMPI_Barrier = dlsym(lib_handles[MPI_HANDLE], "MPI_Barrier");
  pMPI_Bcast = dlsym(lib_handles[MPI_HANDLE], "MPI_Bcast");
  pMPI_Comm_create = dlsym(lib_handles[MPI_HANDLE], "MPI_Comm_create");
  pMPI_Comm_free = dlsym(lib_handles[MPI_HANDLE], "MPI_Comm_free");
  pMPI_Comm_group = dlsym(lib_handles[MPI_HANDLE], "MPI_Comm_group");
  pMPI_Comm_rank = dlsym(lib_handles[MPI_HANDLE], "MPI_Comm_rank");
  pMPI_Comm_size = dlsym(lib_handles[MPI_HANDLE], "MPI_Comm_size");
  pMPI_Comm_split = dlsym(lib_handles[MPI_HANDLE], "MPI_Comm_split");
  pMPI_Errhandler_set = dlsym(lib_handles[MPI_HANDLE], "MPI_Errhandler_set");
  pMPI_Finalize = dlsym(lib_handles[MPI_HANDLE], "MPI_Finalize");
  pMPI_Gather = dlsym(lib_handles[MPI_HANDLE], "MPI_Gather");
  pMPI_Group_free = dlsym(lib_handles[MPI_HANDLE], "MPI_Group_free");
  pMPI_Group_range_incl = dlsym(lib_handles[MPI_HANDLE], "MPI_Group_range_incl");
  pMPI_Init = dlsym(lib_handles[MPI_HANDLE], "MPI_Init");
  pMPI_Recv = dlsym(lib_handles[MPI_HANDLE], "MPI_Recv");
  pMPI_Reduce = dlsym(lib_handles[MPI_HANDLE], "MPI_Reduce");
  pMPI_Send = dlsym(lib_handles[MPI_HANDLE], "MPI_Send");
  pMPI_Wtime = dlsym(lib_handles[MPI_HANDLE], "MPI_Wtime");
  if((rc = (*pMPI_Init)(argcp, argvp)) == MPI_SUCCESS)
    {
      conditional_report(SHOW_ENVIRONMENT, "Using MPI\n");
      use_mpi = YES_MPI;
    }
  else
    {
      conditional_report(SHOW_ENVIRONMENT, "No MPI - MPI_Init failed, attempting to proceed without\n");
      return;
    }
}

void
mpi_recv(void *buf, int count, MPI_Datatype datatype, 
	 int source, int tag, MPI_Comm comm, MPI_Status *status)
{
  int rc = 0;

  if(USE_MPI)
    if((rc = (*pMPI_Recv)(buf, count, datatype, source, tag, comm, status)) != MPI_SUCCESS)
      FAIL();
}

void
mpi_reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
  int rc = 0;
  int size = 1;

  if(USE_MPI)
    {
      if((rc = (*pMPI_Reduce)(sendbuf, recvbuf, count, datatype, op, root, comm)) != MPI_SUCCESS)
	FAIL();
    }
  else
    {
      switch (datatype)
	{
	case MPI_INT :
	  size = 4;
	  break;
	case MPI_DOUBLE : 
	  size = 8;
	  break;
	case MPI_CHAR :
	default :
	  size = 1;
	  break;
	}
      size *= count;
      memcpy(recvbuf, sendbuf, size);
    }
}

void
mpi_send(void *buffer, int count, MPI_Datatype datatype, 
	 int dest, int tag, MPI_Comm comm)
{
  int rc = 0;

  if(USE_MPI)
    if((rc = (*pMPI_Send)(buffer, count, datatype, dest, tag, comm)) != MPI_SUCCESS)
      FAIL();
}

double 
mpi_wtime()
{
  double now;

  if(USE_MPI)
    {
      return((*pMPI_Wtime)());
    }
  else
    {
      now = time(NULL);
      return(now);
    }
}

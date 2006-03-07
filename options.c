/*****************************************************************************\
 *  $Id: options.c,v 1.13 2006/01/09 16:26:39 auselton Exp $
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
#include <stdlib.h>
#include <string.h>     /* strncpy */
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#define _GNU_SOURCE
#include <getopt.h>
#include "sys_wrap.h"
#include "miberr.h"
#include "options.h"
#include "mpi_wrap.h"

Options *Init_Opts();
Options *Make_Opts();
Mib *Init_Mib(int rank, int size);
void command_line_overrides();
void show_keys(Options *opts);

BOOL set_string(char *v, char *str);
BOOL set_flags(char *v, int *flagp, int mask);
BOOL set_int(char *v, int *nump);
BOOL set_longlong(char *v, long long *llnump);

void lowercase(char *str);
BOOL kv_pair(char *buf, char **k, char **v);
BOOL set_key(char *k, char *v, Options *opts);
SLURM *get_SLURM_env();
int get_int_env(char *key);
void get_str_env(char *str, char *key);
void show_SLURM_env(SLURM *slurm);

Options *cl_opts;
int      cl_opts_mask = 0;
Options *opts;
SLURM   *slurm;
Mib     *mib;

void
command_line(int argc, char *argv[], int rank)
{
  int opt;
  char *opt_str = "ad:EIhHl:L:npPrRs:St:W";
  int idx;
  const struct option long_options[] = 
    {
      {"use_node_aves", 0, NULL, 'a'},
      {"log_dir", 1, NULL, 'd'},
      {"show_environment", 0, NULL, 'E'},
      {"show_intermediate_values", 0, NULL, 'I'},
      {"show_headers", 0, NULL, 'H'},
      {"call_limit", 1, NULL, 'l'},
      {"time_limit", 1, NULL, 'L'},
      {"new ", 0 , NULL, 'n'},
      {"profiles", 0, NULL, 'p'},
      {"show_progress", 0, NULL, 'P'},
      {"remove", 0, NULL, 'r'},
      {"read_only", 0, NULL, 'R'},
      {"call_size", 1, NULL, 's'},
      {"show_signon", 0, NULL, 'S'},
      {"test_dir", 1, NULL, 't'},
      {"write_only", 0, NULL, 'W'},
      {"help", 0, NULL, 'h'},
      {0, 0, 0, 0},
    };

  cl_opts = Make_Opts();
  DEBUG("Enter command_line\n");
  while( (opt = getopt_long(argc, argv, opt_str, long_options, &idx)) != -1)
    {
      switch(opt)
	{
	case 'a' : /* use_node_aves */
	  set_flags("true", &(cl_opts->flags), USE_NODE_AVES);
	  set_flags("true", &(cl_opts_mask), CL_USE_NODE_AVES);
	  break;
	case 'd' : /* log_dir */
	  set_string(optarg, cl_opts->log_dir);
	  set_flags("true", &(cl_opts_mask), CL_LOG_DIR);
	  break;
	case 'E' : /* show_environment */
	  set_flags("true", &(cl_opts->verbosity), SHOW_ENVIRONMENT);
	  set_flags("true", &(cl_opts_mask), CL_SHOW_ENVIRONMENT);
	  break;
	case 'I' : /* show_intermediate_values */
	  set_flags("true", &(cl_opts->verbosity), SHOW_INTERMEDIATE_VALUES);
	  set_flags("true", &(cl_opts_mask), CL_SHOW_INTERMEDIATE_VALUES);
	  break;
	case 'H' : /* show_headers */
	  set_flags("true", &(cl_opts->verbosity), SHOW_HEADERS);
	  set_flags("true", &(cl_opts_mask), CL_SHOW_HEADERS);
	  break;
	case 'l' : /* call_limit */
	  set_int(optarg, &(cl_opts->call_limit));
	  set_flags("true", &(cl_opts_mask), CL_CALL_LIMIT);
	  break;
	case 'L' : /* time_limit */
	  set_int(optarg, &(cl_opts->time_limit));
	  set_flags("true", &(cl_opts_mask), CL_TIME_LIMIT);
	  break;
	case 'n' : /* new */
	  set_flags("true", &(cl_opts->flags), NEW);
	  set_flags("true", &(cl_opts_mask), CL_NEW);
	  break;
	case 'p' : /* profiles */
	  set_flags("true", &(cl_opts->flags), PROFILES);
	  set_flags("true", &(cl_opts_mask), CL_PROFILES);
	  break;
	case 'P' : /* show_progress */
	  set_flags("true", &(cl_opts->verbosity), SHOW_PROGRESS);
	  set_flags("true", &(cl_opts_mask), CL_SHOW_PROGRESS);
	  break;
	case 'r' : /* remove */
	  set_flags("true", &(cl_opts->flags), REMOVE);
	  set_flags("true", &(cl_opts_mask), CL_REMOVE);
	  break;
	case 'R' : /* read_only */
	  set_flags("true", &(cl_opts->flags), READ_ONLY);
	  set_flags("true", &(cl_opts_mask), CL_READ_ONLY);
	  break;
	case 's' : /* call_size */
	  set_longlong(optarg, &(cl_opts->call_size));
	  set_flags("true", &(cl_opts_mask), CL_CALL_SIZE);
	  break;
	case 'S' : /* show_signon */
	  set_flags("true", &(cl_opts->verbosity), SHOW_SIGNON);
	  set_flags("true", &(cl_opts_mask), CL_SHOW_SIGNON);
	  break;
	case 't' : /* test_dir */
	  set_string(optarg, opts->testdir);
	  set_flags("true", &(cl_opts_mask), CL_TESTDIR);
	  break;
	case 'W' : /* write_only */
	  set_flags("true", &(cl_opts->flags), WRITE_ONLY);
	  set_flags("true", &(cl_opts_mask), CL_WRITE_ONLY);
	  break;
	case 'h' :  /* help */
	default : if (rank == 0) usage(); break;
	}
    }
  DEBUG("Exit command_line\n");
}

Options *
read_options(int rank, int size)
{
  int opt;
  char *opt_str = "d:h";
  Options *opts;
  
  DEBUG("Enter read_options\n");
  slurm = get_SLURM_env();
  mib = Init_Mib(rank, size);
  opts = Init_Opts();
  show_keys(opts);
  DEBUG("Exit read_options\n");
  return(opts);
}

void
usage( void )
{
  printf("usage: mib [ad:EIhHl:L:npPrRs:St:W]\n");
  printf("       -a                  :  Use average profile times accross node.\n");
  printf("       -d <log_dir>        :  parameters file and profiles files here, \n");
  printf("                           :    if any.\n");
  printf("       -E                  :  Show environment of test in output.\n");
  printf("       -I                  :  Show intermediate values in output.\n");
  printf("       -h                  :  This message\n");
  printf("       -H                  :  Show headers in output.\n");
  printf("       -l <call_limit>     :  Issue no more than this many system calls.\n");
  printf("       -L <time_limit>     :  Do not issue new system calls after this many\n");
  printf("                           :    seconds (limits are per phase for write and\n");
  printf("       -n                  :    read phases).\n");
  printf("       -n                  :  Create new files if files were already present\n");
  printf("                           :    (will always create new files if none were\n");
  printf("       -n                  :    present).\n");
  printf("       -p                  :  Output system call timing profiles to <log_dir>\n");
  printf("                           :    (default is current working directory).\n");
  printf("       -P                  :  Show progress bars during testing.\n");
  printf("       -r                  :  Remove files when done.\n");
  printf("       -R                  :  Only perform the read test.\n");
  printf("       -s <call_size>      :  Use system calls of this size (default 512k).\n");
  printf("                           :    Numbers may use abreviations k, K, m, or M.\n");
  printf("       -S                  :  Show signon message, including date, program\n");
  printf("                           :    name, and version number.\n");
  printf("       -t <test_dir>       :  I/O transactions to and from this directory\n");
  printf("                           :    (default is current working directory).\n");
  printf("       -W                  :  Only perform the Write test\n");
  printf("                           :    (if -R and -W are both present both tests \n");
  printf("                           :     will run, but that's the default anyway)\n");
  exit(0);
}

Options *
Init_Opts()
{
  Options *opts;
  char options[MAX_BUF];
  char buf[MAX_BUF];
  char *b;
  char *k;
  char *v;
  FILE *ofp;
  int ret;
  int len = 0;
  int send;
  MPI_Comm subcomm;
  MPI_Group world_group, group;
  int group_spec[3];

  DEBUG("Enter Init_Opts\n");
  opts = Make_Opts();
  if(mib->rank == mib->base)
    {
      if (check_cl(CL_LOG_DIR))
	{
	  strncpy(opts->log_dir, cl_opts->log_dir, MAX_BUF);
	  snprintf(options, MAX_BUF, "%s/options", opts->log_dir);
	  printf("Reading options from log dir %s\n", options);
	  fflush(stdout);
	  if( (ofp = fopen(options, "r")) != NULL)
	    { 
	      b = Fgets(buf, MAX_BUF, ofp);
	      while( b != NULL)
		{
		  if (kv_pair(buf, &k, &v))
		    if(!set_key(k, v, opts))
		      FAIL();
		  b = Fgets(buf, MAX_BUF, ofp);
		}
	      fclose(ofp);
	    }
	  if( (check_flags(WRITE_ONLY))  && (check_flags(READ_ONLY)) )
	    {
	      opts->flags &= ~WRITE_ONLY;
	      opts->flags &= ~READ_ONLY;
	    }
	}
      
      while(opts->testdir[len++]);
      if( mib->tasks == mib->nodes ) opts->flags &= ~USE_NODE_AVES;
      command_line_overrides(opts);
    }
  mpi_bcast(&(len), 1, MPI_INT, mib->base, mib->comm);
  mpi_bcast(opts->testdir, len, MPI_CHAR, mib->base, mib->comm);
  mpi_bcast(&(opts->call_limit), 1, MPI_INT, mib->base, mib->comm);
  mpi_bcast(&(opts->call_size), 1, MPI_LONG_LONG, mib->base, mib->comm);
  mpi_bcast(&(opts->time_limit), 1, MPI_INT, mib->base, mib->comm);
  mpi_bcast(&(opts->flags), 1, MPI_INT, mib->base, mib->comm);
  mpi_bcast(&(opts->verbosity), 1, MPI_INT, mib->base, mib->comm);
  
  if(mib->tasks < mib->size)
    {
      MPI_Comm_group(MPI_COMM_WORLD, &world_group);
      group_spec[0] = mib->base;
      group_spec[1] = mib->tasks + mib->base - 1;
      group_spec[3] = 1;
      MPI_Group_range_incl(world_group, 1, &group_spec, &group);
      MPI_Comm_create(MPI_COMM_WORLD, group, &(mib->comm));
    }
  DEBUG("Exit Init_Opts\n");
  return(opts);
}

Options *
Make_Opts()
{
  Options *opts;

  DEBUG("Enter Make_Opts\n");
  opts = (Options *)Malloc(sizeof(Options));
  opts->log_dir[0] = '\0';
  strncpy(opts->testdir, ".", MAX_BUF);
  opts->call_limit = 4096;
  opts->call_size = 524288;
  opts->time_limit = 60;
  opts->flags = DEFAULTS;
  opts->verbosity = QUIET;
  DEBUG("Exit Make_Opts\n");
  return(opts);
}

Mib *
Init_Mib(int rank, int size)
{
  Mib *mib;

  mib = (Mib *)Malloc(sizeof(Mib));
  mib->nodes = slurm->NNODES;
  mib->tasks = slurm->NPROCS;
  mib->rank = rank;
  mib->size = size;
  mib->base = 0;
  mib->comm = MPI_COMM_WORLD;
}

void
Free_Opts()
{
  if(opts != NULL)
    free(opts);
}



void 
command_line_overrides(Options *opts)
{
  ASSERT(mib->rank == mib->base);
  if(check_cl(CL_TESTDIR))
    set_string(cl_opts->testdir, opts->testdir);
  if(check_cl(CL_CALL_LIMIT))
    opts->call_limit = cl_opts->call_limit;
  if(check_cl(CL_CALL_SIZE))
    opts->call_size = cl_opts->call_size;
  if(check_cl(CL_TIME_LIMIT))
    opts->time_limit = cl_opts->time_limit;
  if(check_cl(CL_NEW))
    set_flags("true", &(opts->flags),NEW );
  if(check_cl(CL_REMOVE))
    set_flags("true", &(opts->flags), REMOVE);
  if(check_cl(CL_WRITE_ONLY))
    set_flags("true", &(opts->flags), WRITE_ONLY);
  if(check_cl(CL_READ_ONLY))
    set_flags("true", &(opts->flags), READ_ONLY);
  if(check_cl(CL_PROFILES))
    set_flags("true", &(opts->flags), PROFILES);
  if(check_cl(CL_USE_NODE_AVES))
    set_flags("true", &(opts->flags), USE_NODE_AVES);
  if(check_cl(CL_SHOW_SIGNON))
    set_flags("true", &(opts->verbosity), SHOW_SIGNON);
  if(check_cl(CL_SHOW_HEADERS))
    set_flags("true", &(opts->verbosity), SHOW_HEADERS);
  if(check_cl(CL_SHOW_ENVIRONMENT))
    set_flags("true", &(opts->verbosity), SHOW_ENVIRONMENT);
  if(check_cl(CL_SHOW_PROGRESS))
    set_flags("true", &(opts->verbosity), SHOW_PROGRESS);
  if(check_cl(CL_SHOW_INTERMEDIATE_VALUES))
    set_flags("true", &(opts->verbosity), SHOW_INTERMEDIATE_VALUES);
}

BOOL
kv_pair(char *buf, char **k, char **v)
{
  char *eq;
  char *end;

  ASSERT(buf != NULL);
  *k = NULL;
  *v = NULL;

  eq = buf;
  while( (*eq != '#') && (*eq != '=') && (*eq != '\0') && (eq < buf + MAX_BUF - 1) ) eq++;
  if( (*eq == '#') || (*eq == '\0') || (eq == buf + MAX_BUF -1) ) return(FALSE);

  *k = buf;
  while( isspace(**k) && (*k < eq) ) (*k)++;
  if ( *k == eq) return(FALSE);

  end = *k + 1;
  while( ! isspace(*end) && (end < eq) ) end++;
  *end = '\0';

  end++;
  while( (*end != '#') && (*end != '\0') && (*end != '\n') && (end < buf + MAX_BUF - 1) ) end++;
  if ( (*end == '#') || (*end == '\n') )  end--;
 
  while( isspace(*end) ) end--;
  if (end == eq) return(FALSE);
  end++;
  *end = '\0';

  *v = eq + 1;
  while( isspace(**v) && (*v < end) ) (*v)++;
  return(TRUE);
}

BOOL
set_key(char *k, char *v, Options *opts)
{
  lowercase(k);
  if(strncmp(k, "new", MAX_BUF) == 0)
    return(set_flags(v, &(opts->flags), NEW));
  if(strncmp(k, "remove", MAX_BUF) == 0)
    return(set_flags(v, &(opts->flags), REMOVE));
  if(strncmp(k, "testdir", MAX_BUF) == 0)
    return(set_string(v, opts->testdir));
  if(strncmp(k, "call_limit", MAX_BUF) == 0)
    return(set_int(v, &(opts->call_limit)));
  if(strncmp(k, "call_size", MAX_BUF) == 0)
    return(set_longlong(v, &(opts->call_size)));
  if(strncmp(k, "time_limit", MAX_BUF) == 0)
    return(set_int(v, &(opts->time_limit)));
  if(strncmp(k, "write_only", MAX_BUF) == 0)
    return(set_flags(v, &(opts->flags), WRITE_ONLY));
  if(strncmp(k, "read_only", MAX_BUF) == 0)
    return(set_flags(v, &(opts->flags), READ_ONLY));
  if(strncmp(k, "profiles", MAX_BUF) == 0)
    return(set_flags(v, &(opts->flags), PROFILES));
  if(strncmp(k, "use_node_aves", MAX_BUF) == 0)
    return(set_flags(v, &(opts->flags), USE_NODE_AVES));
  if(strncmp(k, "show_signon", MAX_BUF) == 0)
    return(set_flags(v, &(opts->verbosity), SHOW_SIGNON));
  if(strncmp(k, "show_headers", MAX_BUF) == 0)
    return(set_flags(v, &(opts->verbosity), SHOW_HEADERS));
  if(strncmp(k, "show_environment", MAX_BUF) == 0)
    return(set_flags(v, &(opts->verbosity), SHOW_ENVIRONMENT));
  if(strncmp(k, "show_progress", MAX_BUF) == 0)
    return(set_flags(v, &(opts->verbosity), SHOW_PROGRESS));
  if(strncmp(k, "show_intermediate_values", MAX_BUF) == 0)
    return(set_flags(v, &(opts->verbosity), SHOW_INTERMEDIATE_VALUES));
  printf("Unrecognized options key \"%s\"\n", k);
  return(FALSE);
}

void
show_keys(Options *opts)
{
  char cluster[MAX_BUF];
  char *p;

  ASSERT(opts != NULL);
  if ( (mib->rank == mib->base) && (opts->verbosity & SHOW_ENVIRONMENT) )
    {
      strncpy(cluster, slurm->SRUN_COMM_HOST, MAX_BUF);
      for(p = cluster; (*p != '\0') && ( p - cluster < MAX_BUF ) ; p++);
      if( (p > cluster) && (p - cluster < MAX_BUF)  && (*(p-1) == 'i') ) *(p-1) = '\0';
      printf("cluster                  = %s\n", cluster);      
      printf("new                      = %s\n", ((opts->flags & NEW) ? "true" : "false"));
      printf("remove                   = %s\n", ((opts->flags & REMOVE) ? "true" : "false"));
      printf("nodes                    = %d\n", mib->nodes);
      printf("testdir                  = %s\n", opts->testdir);
      printf("call_limit               = %d\n", opts->call_limit);
      printf("call_size                = %lld\n", opts->call_size);
      printf("time_limit               = %d\n", opts->time_limit);
      printf("tasks                    = %d\n", mib->tasks);
      printf("write_only               = %s\n", ((opts->flags & WRITE_ONLY) ? "true" : "false"));
      printf("read_only                = %s\n", ((opts->flags & READ_ONLY) ? "true" : "false"));
      printf("profiles                 = %s\n", ((opts->flags & PROFILES) ? "true" : "false"));
      printf("use_node_aves            = %s\n", ((opts->flags & USE_NODE_AVES) ? "true" : "false"));
      printf("show_signon              = %s\n", ((opts->verbosity & SHOW_SIGNON) ? "true" : "false"));
      printf("show_headers             = %s\n", ((opts->verbosity & SHOW_HEADERS) ? "true" : "false"));
      printf("show_environment         = %s\n", ((opts->verbosity & SHOW_ENVIRONMENT) ? "true" : "false"));
      printf("show_progress            = %s\n", ((opts->verbosity & SHOW_PROGRESS) ? "true" : "false"));
      printf("show_intermediate_values = %s\n", ((opts->verbosity & SHOW_INTERMEDIATE_VALUES) ? "true" : "false"));
      fflush(stdout);
    }
}

BOOL
set_string(char *v, char *str)
{
  int ret;

  if ( (ret = snprintf(str, MAX_BUF, "%s", v)) < 0)
    FAIL();
  return(TRUE);
}

BOOL
set_flags(char *v, int *flagp, int mask)
{
  if( (v[0] == 't') ||
      (v[0] == 'y') ||
      (v[0] == '1') )
    {
      *flagp |= mask;
      return(TRUE);
    }
  if( (v[0] == 'f') ||
      (v[0] == 'n') ||
      (v[0] == '0') )
    {
      *flagp &= ~mask;
      return(TRUE);
    }
  return(FALSE);
}

BOOL
set_int(char *v, int *nump)
{
  *nump = atoi(v);
  return(TRUE);
}

BOOL
set_longlong(char *v, long long *llnump)
{
  int ret;
  char c;

  ret = sscanf(v, "%lld %c", llnump, &c);
  if (ret < 1) FAIL();
  if(ret > 1)
    {
      switch((int)c)
	{
	case 'k':
	case 'K': (*llnump) *= 1024;
	  break;
	case 'm':
	case 'M': (*llnump) *= 1024*1024;
	  break;
	default: break;
	}
    }	  
  return(TRUE);
}

void
lowercase(char *str)
{
  while(*str)
    {
      if( (*str >= 'A') && (*str <= 'Z') )
	*str += 'a' - 'A';
      str++;
    }
}

SLURM *
get_SLURM_env()
{
  SLURM *slurm;
  
  DEBUG("Enter get_SLURM_env\n");
  slurm = (SLURM*)Malloc(sizeof(SLURM));
  slurm->CPUS_ON_NODE = get_int_env("SLURM_CPUS_ON_NODE");
  slurm->CPUS_PER_TASK = get_int_env("SLURM_CPUS_PER_TASK");
  get_str_env(slurm->CPU_BIND_LIST, "SLURM_CPU_BIND_LIST");
  get_str_env(slurm->CPU_BIND_TYPE, "SLURM_CPU_BIND_TYPE");
  get_str_env(slurm->CPU_BIND_VERBOSE, "SLURM_CPU_BIND_VERBOSE");
  slurm->JOBID = get_int_env("SLURM_JOBID");
  get_str_env(slurm->LAUNCH_NODE_IPADDR, "SLURM_LAUNCH_NODE_IPADDR");
  slurm->LOCALID = get_int_env("SLURM_LOCALID");
  slurm->NNODES = get_int_env("SLURM_NNODES");
  slurm->NODEID = get_int_env("SLURM_NODEID");
  get_str_env(slurm->NODELIST, "SLURM_NODELIST");
  slurm->NPROCS = get_int_env("SLURM_NPROCS");
  slurm->PROCID = get_int_env("SLURM_PROCID");
  get_str_env(slurm->SRUN_COMM_HOST, "SLURM_SRUN_COMM_HOST");
  slurm->SRUN_COMM_PORT = get_int_env("SLURM_SRUN_COMM_PORT");
  slurm->STEPID = get_int_env("SLURM_STEPID");
  get_str_env(slurm->TASKS_PER_NODE, "SLURM_TASKS_PER_NODE");
  slurm->TASK_PID = get_int_env("SLURM_TASK_PID");
  get_str_env(slurm->UMASK, "SLURM_UMASK");
  /*  show_SLURM_env(slurm); */
  DEBUG("Exit get_SLURM_env\n");
  return(slurm);
}

void
show_SLURM_env(SLURM *slurm)
{
  printf("SLURM_CPUS_ON_NODE = %d\n", slurm->CPUS_ON_NODE);
  printf("SLURM_CPUS_PER_TASK = %d\n", slurm->CPUS_PER_TASK);
  printf("SLURM_CPU_BIND_LIST = %s\n", slurm->CPU_BIND_LIST);
  printf("SLURM_CPU_BIND_TYPE = %s\n", slurm->CPU_BIND_TYPE);
  printf("SLURM_CPU_BIND_VERBOSE = %s\n", slurm->CPU_BIND_VERBOSE);
  printf("SLURM_JOBID = %d\n", slurm->JOBID);
  printf("SLURM_LAUNCH_NODE_IPADDR = %s\n", slurm->LAUNCH_NODE_IPADDR);
  printf("SLURM_LOCALID = %d\n", slurm->LOCALID);
  printf("SLURM_NNODES = %d\n", slurm->NNODES);
  printf("SLURM_NODEID = %d\n", slurm->NODEID);
  printf("SLURM_NODELIST = %s\n", slurm->NODELIST);
  printf("SLURM_NPROCS = %d\n", slurm->NPROCS);
  printf("SLURM_PROCID = %d\n", slurm->PROCID);
  printf("SLURM_SRUN_COMM_HOST = %s\n", slurm->SRUN_COMM_HOST);
  printf("SLURM_SRUN_COMM_PORT = %d\n", slurm->SRUN_COMM_PORT);
  printf("SLURM_STEPID = %d\n", slurm->STEPID);
  printf("SLURM_TASKS_PER_NODE = %s\n", slurm->TASKS_PER_NODE);
  printf("SLURM_TASK_PID = %d\n", slurm->TASK_PID);
  printf("SLURM_UMASK = %s\n", slurm->UMASK);
}

int
get_int_env(char *key)
{
  char *val;

  if( (val = getenv(key)) != NULL )
    {
      /*      printf( "%s = %s\n", key, val);
	      fflush(stdout);*/
      return(atoi(val));
    }
  return(0);
}
  
void
get_str_env(char *str, char *key)
{
  char *val;

  if( (val = getenv(key)) != NULL )
    {
      /*      printf( "%s = %s\n", key, val);
	      fflush(stdout);*/
      strncpy(str, val, MAX_BUF);
    }
  else
    str[0] = '\0';
}
  

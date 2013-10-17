/*
    Copyright 2011, Dan Gibson.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <sys/unistd.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sched.h>

#include "ProcessorMap.h" 
#define fatal

ProcessorMap::ProcessorMap() { 
  m_nProcs = 0;
  m_p_nProcessor_Ids = NULL;

  m_nProcs = DetermineNumberOfProcessors();
  if( m_nProcs <= 0 ) {
    fatal("sched_getaffinity() reports empty processor mask.\n");
  }

  m_p_nProcessor_Ids = new int[m_nProcs];
  if(m_p_nProcessor_Ids == NULL ) {
    fatal("new int[%i] returned NULL -- out of memory?\n", m_nProcs );
  }

  int i;
  int n = 0;

  cpu_set_t cpus;

  // Returns number of processors available to process (based on affinity mask)
  if( sched_getaffinity(0, sizeof(cpus), (cpu_set_t*) &cpus) < 0) {
    fatal("sched_getaffinity() reports empty processor mask.\n" );
  }

  for (i = 0; n<m_nProcs && i < sizeof(cpus)*8; i++) {
    if( CPU_ISSET( i, &cpus ) ) {
      m_p_nProcessor_Ids[n] = i;
      n++;
    }
  }


  if( n != m_nProcs ) {
    fatal("Unable to find all processor numbers.\n" );
  }

} 

ProcessorMap::~ProcessorMap() { 
  if( m_p_nProcessor_Ids != NULL ) {
    delete [] m_p_nProcessor_Ids;
    m_p_nProcessor_Ids = NULL;
  }
} 

int ProcessorMap::LogicalToPhysical(int lproc) const { 
  IntegrityCheck();
  if( lproc < 0 || lproc >= m_nProcs ) {
    fatal( "Logical processor number out of range: [%i,%i) (%i)",0,m_nProcs,lproc);
  }

  return m_p_nProcessor_Ids[lproc];
} 

int ProcessorMap::PhysicalToLogical(int pproc) const { 
  IntegrityCheck();

  int i;
  for(i=0;i<m_nProcs;i++) {
    if( m_p_nProcessor_Ids[i] == pproc ) break;
  }

  if( i == m_nProcs ) {
    fatal( "Physical processor number does not match any known physical processor numbers.");
  }

  return i;
} 

void ProcessorMap::IntegrityCheck( ) const {
  if( m_nProcs == 0 || m_p_nProcessor_Ids == NULL ) {
    fatal( "Processor Map not in a usable state." );
  }
} 

/* Borrowed from the Phoenix MapReduce runtime */
int ProcessorMap::DetermineNumberOfProcessors() {
   int nProcs = 0;
   cpu_set_t cpus;
   
   // Returns number of processors available to process (based on affinity mask)
   if( sched_getaffinity(0, sizeof(cpus), (cpu_set_t*) &cpus) < 0) {
     nProcs = -1;
     CPU_ZERO( &cpus );
   }

   for (unsigned i = 0; i < sizeof(cpus)*8; i++) {
      if( CPU_ISSET( i, &cpus )) {
        nProcs++;
      }
   }
  return nProcs;
}

void ProcessorMap::BindToPhysicalCPU( int pproc ) const {
  /* Verify pproc is in the physical cpu array */
  int lcpu = -1;
  for( int i=0; i<m_nProcs;i++ ) {
    if( m_p_nProcessor_Ids[i] == pproc ) {
      lcpu = i;
      break;
    }
  }

  if( lcpu != -1 ) {
    cpu_set_t myProc;
    CPU_ZERO( &myProc );
    CPU_SET( pproc, &myProc );

    if( sched_setaffinity(0, sizeof(myProc), &myProc) < 0 ) {
      fatal("Call to sched_setaffinity() failed for physical CPU %i\n",pproc);
    }
  } else {
    fatal("Failed to bind to processor %i\n -- Processor does not exist!",pproc );
  }
}


/* Function similar to MPI_Alltoallv except that 
 * the receiver does not need to know the exact
 * displacements and lengths beforehand.
 * Instead, only a maximum total length needs to
 * be given. The received data is concatenated
 * parameters are used as in MPI_ALLTOALLV except:
  IN   maxRCount   specifies the maximum total 
                   number of items (of type recvtype)
                   that would fit into recvbuf
  OUT  size        total number of received items (not bytes)
  OUT  recvcounts  
  OUT  rdispls     number of elements and displacements
                   for each process - as in MPI_Alltoallv - 
                   but they are an output now 
*/
int my_Alltoallv(void *sendbuf, int *sendcounts, int *sdispls,
                 MPI_Datatype sendtype, void *recvbuf, int maxrcount,int *size,
                 int *recvcounts, int *rdispls, MPI_Datatype recvtype,
                 MPI_Comm comm)
{
...
}

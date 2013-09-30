
      block data prob
      parameter (prbdim=3)
      common /prblm/ mat, q
      double precision mat(prbdim,prbdim), q(prbdim)
      data mat /4, 1, -1, 1, 4, 1, -1, 1, 4/
      data q /3, 31, -10/
      end

      program linmcp
      parameter (prbdim=3)
      integer n, nnz, i, status
      double precision z(prbdim), f(prbdim), lb(prbdim), ub(prbdim)

      n = prbdim
      nnz = prbdim*prbdim

      do 10 i=1,n
         lb(i) = 1.0
         ub(i) = 10.0
	 z(i) = 5.0
 10   continue
      
      call pathmain(n, nnz, status, z, f, lb, ub)

      stop
      end

      integer function funceval(n,z,f)
      integer n
      double precision z(n), f(n)

      parameter (prbdim=3)
      common /prblm/ mat, q
      double precision mat(prbdim,prbdim), q(prbdim)

      integer i, j

      do 10 i=1,n
         f(i) = q(i)
 10   continue
      do 30 i=1,n
         do 20 j=1,n
            f(i) = f(i) + mat(i,j)*z(j)
 20      continue
 30   continue
      funceval = 0
      return
      end

      integer function jaceval(n,nnz,z,col,len,row,data)
      integer n, nnz
      double precision z(n), data(nnz)
      integer col(n), len(n), row(nnz)

      parameter (prbdim=3)
      common /prblm/ mat,q
      double precision mat(prbdim,prbdim), q(prbdim)

      integer i, j

      do 20 j=1,n
         col(j) = (j-1)*prbdim + 1
         len(j) = prbdim
         do 10 i=1,n
            row(i + (j-1)*prbdim) = i
 10      continue
 20   continue

      do 40 i=1,prbdim
         do 30 j=1,prbdim
            data(prbdim*(j-1)+i) = mat(i,j)
 30      continue
 40   continue

      jaceval = 0
      return
      end


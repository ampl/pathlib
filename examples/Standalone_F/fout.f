
      subroutine fout(unitnum, messag)
      integer unitnum
      character*(*) messag

      write (unitnum, 10) messag

 10   format (a)
      return
      end


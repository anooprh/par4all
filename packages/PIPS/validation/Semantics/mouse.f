      program mouse
      integer entry
      common /centry/ entry
      data entry / 0 /
      mode = 0
      call trap(mode)
      mode = 1
      call trap(mode)
      end
      subroutine trap(i)
      integer entry
      common /centry/ entry
      if(entry.le.0) then
         entry = 1
      endif
      print *, i
      end

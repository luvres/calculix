!
!     CalculiX - A 3-dimensional finite element program
!              Copyright (C) 1998-2018 Guido Dhondt
!
!     This program is free software; you can redistribute it and/or
!     modify it under the terms of the GNU General Public License as
!     published by the Free Software Foundation(version 2);
!     
!
!     This program is distributed in the hope that it will be useful,
!     but WITHOUT ANY WARRANTY; without even the implied warranty of 
!     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
!     GNU General Public License for more details.
!
!     You should have received a copy of the GNU General Public License
!     along with this program; if not, write to the Free Software
!     Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
!
      subroutine hrv_ud(nface,ielfa,vel,gradvel,gamma,xlet,
     &  xxn,xxj,ipnei,betam,nef,flux,vfa)
!
!     determine gamma for the temperature:
!        upwind difference: gamma=0
!        central difference: gamma=1
!
      implicit none
!
      integer nface,ielfa(4,*),i,j,indexf,ipnei(*),iel1,iel2,nef
!
      real*8 vel(nef,0:7),gradvel(3,3,*),xxn(3,*),xxj(3,*),vud,vcd,
     &  gamma(*),phic,xlet(*),betam,flux(*),vfa(0:7,*)
!
c$omp parallel default(none)
c$omp& shared(nface,ielfa,ipnei,vel,vfa,flux)
c$omp& private(i,iel2,iel1,j,indexf)
c$omp do
      do i=1,nface
         iel2=ielfa(2,i)
!
!        faces with only one neighbor need not be treated
!
         if(iel2.le.0) cycle
         iel1=ielfa(1,i)
         j=ielfa(4,i)
         indexf=ipnei(iel1)+j
!
         if(flux(indexf).ge.0.d0) then
!
            do j=1,3
               vfa(j,i)=vel(iel1,j)
            enddo
         else
!
            do j=1,3
               vfa(j,i)=vel(iel2,j)
            enddo
         endif
      enddo
c$omp end do
c$omp end parallel
!            
      return
      end

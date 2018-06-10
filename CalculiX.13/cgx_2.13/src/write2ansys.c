/* ---------------------------------------------------------------------------------------------  */
/* write2ansys schreibt das frd-file aus einer struktur  7.11.1998 Wittig   */
/* ---------------------------------------------------------------------------------------------  */

#include <extUtil.h>


int write2ansys(char *datout, Summen *anz, Nodes *node, Elements *elem, Datasets *lcase, char **dat )
{
  FILE *handle1=NULL, *handle2=NULL;
  int  i, lc, lendatout;

  /* Open the files and check to see that it was opened correctly */

  printf (" write ansys data \n");

  lendatout=strlen(datout);

  /* open the mesh file */
  if(dat==0)
  {
    strcpy (&datout[lendatout], ".msh");
    handle1 = fopen (datout, "w+b");
    if (handle1==NULL) { printf ("\nThe output file \"%s\" could not be opened.\n\n",
       datout); return(-1);}
    else  printf (" file %s opened\n",datout);
    
    printf ("\n write ansys file  \n");
    fprintf (handle1, "/PREP7\n");
  
    for (i=0; i<anz->n; i++)
    {
      /*
      fprintf (handle1, "N, %8d,", node[i].nr );
      betr= sqrt(node[node[i].nr].nx*node[node[i].nr].nx);
      if (betr > 9999.) fprintf (handle1, "%8.1f, ", node[node[i].nr].nx);
      else if (betr > 999.) fprintf (handle1, "%8.2f, ", node[node[i].nr].nx);
      else if (betr > 99.) fprintf (handle1, "%8.3f, ", node[node[i].nr].nx);
      else
          fprintf (handle1, "%8.3f, ", node[node[i].nr].nx);
  
      betr= sqrt(node[node[i].nr].ny*node[node[i].nr].ny);
      if (betr > 9999.) fprintf (handle1, "%8.1f, ", node[node[i].nr].ny);
      else if (betr > 999.) fprintf (handle1, "%8.2f, ", node[node[i].nr].ny);
      else if (betr > 99.) fprintf (handle1, "%8.3f, ", node[node[i].nr].ny);
      else
          fprintf (handle1, "%8.3f, ", node[node[i].nr].ny);
  
      betr= sqrt(node[node[i].nr].nz*node[node[i].nr].nz);
      if (betr > 9999.) fprintf (handle1, "%8.1f, ", node[node[i].nr].nz);
      else if (betr > 999.) fprintf (handle1, "%8.2f, ", node[node[i].nr].nz);
      else if (betr > 99.) fprintf (handle1, "%8.3f, ", node[node[i].nr].nz);
      else
          fprintf (handle1, "%8.3f, ", node[node[i].nr].nz);
  
      fprintf (handle1, "%3.1f, %3.1f, %3.1f\n", 0., 0., 0. );
      */
  
      fprintf (handle1, "N,%d,%.12e,%.12e,%.12e,%.12e,%.12e,%.12e\n", node[i].nr, node[node[i].nr].nx, node[node[i].nr].ny, node[node[i].nr].nz, 0., 0., 0. );
    }
    fprintf (handle1, "SHPP, OFF\n");
    fprintf (handle1, "REAL, 1\n");
  
    for (i=0; i<anz->e; i++)
    {
      if (elem[i].type == 1)
      {
        if (((i>0)&&(elem[i-1].type != elem[i].type))||(i==0))
        {
          // linear hexahedra
          //fprintf (handle1, "ET, 1, 45\n");
          fprintf (handle1, "ET, 1, 185\n");
          fprintf (handle1, "TYPE, 1\n");
        }
        fprintf (handle1, "EN,%8d,%8d,%8d,%8d,%8d,%8d,%8d,%8d,%8d \n",
          elem[i].nr, elem[i].nod[0], elem[i].nod[1], elem[i].nod[2], elem[i].nod[3],
          elem[i].nod[4], elem[i].nod[5], elem[i].nod[6], elem[i].nod[7] );
      }
      else if (elem[i].type == 2)
      {
        if (((i>0)&&(elem[i-1].type != elem[i].type))||(i==0))
        {
          // linear penta -> he8
          fprintf (handle1, "ET, 2, 185\n");
          fprintf (handle1, "TYPE, 2\n");
        }
        fprintf (handle1, "EN,%8d,%8d,%8d,%8d,%8d,%8d,%8d,%8d,%8d \n",
          elem[i].nr, elem[i].nod[0], elem[i].nod[1], elem[i].nod[2], elem[i].nod[2],
          elem[i].nod[3], elem[i].nod[4], elem[i].nod[5], elem[i].nod[5] );
      }
      else if (elem[i].type == 3)
      {
        if (((i>0)&&(elem[i-1].type != elem[i].type))||(i==0))
        {
          // linear tet
          fprintf (handle1, "ET, 3, 285\n");
          fprintf (handle1, "TYPE, 3\n");
        }
        fprintf (handle1, "EN,%8d,%8d,%8d,%8d,%8d \n",
          elem[i].nr, elem[i].nod[0], elem[i].nod[1], elem[i].nod[2], elem[i].nod[3] );
      }
      else if (elem[i].type == 4)
      {
        if (((i>0)&&(elem[i-1].type != elem[i].type))||(i==0))
        {
          // quadratic hexahedra
          //fprintf (handle1, "ET, 4, 95\n");
          fprintf (handle1, "ET, 4, 186\n");
          fprintf (handle1, "TYPE, 4\n");
        }
        fprintf (handle1, "EN,%8d,%8d,%8d,%8d,%8d,%8d,%8d,%8d,%8d \n",
          elem[i].nr, elem[i].nod[0], elem[i].nod[1], elem[i].nod[2], elem[i].nod[3],
          elem[i].nod[4], elem[i].nod[5], elem[i].nod[6], elem[i].nod[7] );
        fprintf (handle1, "EMORE,%8d,%8d,%8d,%8d,%8d,%8d,%8d,%8d \n",
          elem[i].nod[8],  elem[i].nod[9], elem[i].nod[10], elem[i].nod[11],
          elem[i].nod[16], elem[i].nod[17], elem[i].nod[18], elem[i].nod[19] );
        fprintf (handle1, "EMORE,%8d,%8d,%8d,%8d \n",
          elem[i].nod[12], elem[i].nod[13], elem[i].nod[14], elem[i].nod[15] );
      }
      else if (elem[i].type == 5)
      {
        if (((i>0)&&(elem[i-1].type != elem[i].type))||(i==0))
        {
          // quadratic penta -> hexahedra
          fprintf (handle1, "ET, 5, 186\n");
          fprintf (handle1, "TYPE, 5\n");
        }
        fprintf (handle1, "EN,%8d,%8d,%8d,%8d,%8d,%8d,%8d,%8d,%8d \n",
          elem[i].nr, elem[i].nod[0], elem[i].nod[1], elem[i].nod[2], elem[i].nod[2],
          elem[i].nod[3], elem[i].nod[4], elem[i].nod[5], elem[i].nod[5] );
        fprintf (handle1, "EMORE,%8d,%8d,%8d,%8d,%8d,%8d,%8d,%8d \n",
          elem[i].nod[6],  elem[i].nod[7], elem[i].nod[2], elem[i].nod[8],
          elem[i].nod[12], elem[i].nod[13], elem[i].nod[5], elem[i].nod[14] );
        fprintf (handle1, "EMORE,%8d,%8d,%8d,%8d \n",
          elem[i].nod[9], elem[i].nod[10], elem[i].nod[11], elem[i].nod[12] );
      }
      else if (elem[i].type == 6)
      {
        if (((i>0)&&(elem[i-1].type != elem[i].type))||(i==0))
        {
          // quadratic tetrahedra
          //fprintf (handle1, "ET, 6, 42\n");
          fprintf (handle1, "ET, 6, 187\n");
          fprintf (handle1, "TYPE, 6\n");
        }
        fprintf (handle1, "EN,%8d,%8d,%8d,%8d,%8d,%8d,%8d,%8d,%8d \n",
          elem[i].nr, elem[i].nod[0], elem[i].nod[1], elem[i].nod[2], elem[i].nod[3],
          elem[i].nod[4], elem[i].nod[5], elem[i].nod[6], elem[i].nod[7] );
        fprintf (handle1, "EMORE,%8d,%8d \n",
          elem[i].nod[8], elem[i].nod[9]);
      }
      else if (elem[i].type == 7)
      {
        if (((i>0)&&(elem[i-1].type != elem[i].type))||(i==0))
        {
          //fprintf (handle1, "ET, 7, 42\n");
          fprintf (handle1, "ET, 7, 181\n");
          fprintf (handle1, "TYPE, 7\n");
        }
        fprintf (handle1, "EN,%8d,%8d,%8d,%8d,%8d \n",
          elem[i].nr, elem[i].nod[0], elem[i].nod[1], elem[i].nod[2], elem[i].nod[2] );
      }
      else if (elem[i].type == 8)
      {
        if (((i>0)&&(elem[i-1].type != elem[i].type))||(i==0))
        {
          //fprintf (handle1, "ET, 8, 99\n");
          fprintf (handle1, "ET, 10, 281\n");
          fprintf (handle1, "TYPE, 8\n");
        }
        fprintf (handle1, "EN,%8d,%8d,%8d,%8d,%8d,%8d,%8d,%8d,%8d \n",
          elem[i].nr, elem[i].nod[0], elem[i].nod[1], elem[i].nod[2], elem[i].nod[2], elem[i].nod[3], elem[i].nod[4], elem[i].nod[2], elem[i].nod[5] );
      }
      else if (elem[i].type == 9)
      {
        if (((i>0)&&(elem[i-1].type != elem[i].type))||(i==0))
        {
          //fprintf (handle1, "ET, 9, 42\n");
          fprintf (handle1, "ET, 9, 181\n");
          fprintf (handle1, "TYPE, 9\n");
        }
        fprintf (handle1, "EN,%8d,%8d,%8d,%8d,%8d \n",
          elem[i].nr, elem[i].nod[0], elem[i].nod[1], elem[i].nod[2], elem[i].nod[3] );
      }
      else if (elem[i].type == 10)
      {
        if (((i>0)&&(elem[i-1].type != elem[i].type))||(i==0))
        {
          //fprintf (handle1, "ET, 10, 99\n");
          fprintf (handle1, "ET, 10, 281\n");
          fprintf (handle1, "TYPE, 10\n");
        }
        fprintf (handle1, "EN,%8d,%8d,%8d,%8d,%8d,%8d,%8d,%8d,%8d \n",
          elem[i].nr, elem[i].nod[0], elem[i].nod[1], elem[i].nod[2], elem[i].nod[3], elem[i].nod[4], elem[i].nod[5], elem[i].nod[6], elem[i].nod[7]);
      }
      else if (elem[i].type == 11)
      {
        if (((i>0)&&(elem[i-1].type != elem[i].type))||(i==0))
        {
          // 3-D linear beam
          fprintf (handle1, "TYPE, 11\n");
        }
        fprintf (handle1, "EN,%8d,%8d,%8d \n",
          elem[i].nr, elem[i].nod[0], elem[i].nod[1] );
      }
      else if (elem[i].type == 12)
      {
        if (((i>0)&&(elem[i-1].type != elem[i].type))||(i==0))
        {
          // 3-D quadratic beam
          fprintf (handle1, "TYPE, 12\n");
        }
        fprintf (handle1, "EN,%8d,%8d,%8d,%8d \n",
          elem[i].nr, elem[i].nod[0], elem[i].nod[1], elem[i].nod[2] );
      }
      else
      {
        printf (" elem not a known type (%d)\n",  elem[i].type);
      }
    }
    fprintf (handle1, "SHPP, ON\n");
    fprintf (handle1, "CHECK,ESEL\n");
    fclose(handle1);
  }

  /* open node-value file */
  else if((anz->l>0)&&(dat!=NULL))
  {
    if(strlen(dat[0])>0)
    {
      if(compare( dat[0], "ds", 2)== 2)
      {
        if((dat[1]!=(char *)NULL)&&(dat[1][0]=='e'))  sprintf( &datout[lendatout], "_%s%s.dat", dat[0], dat[1]);
        else  sprintf( &datout[lendatout], "_%s.dat", dat[0]);
      }
      else strcpy (&datout[lendatout], ".dat");
      handle2 = fopen (datout, "w+b");
      if (handle2==NULL) { printf ("\nThe output file \"%s\" could not be opened.\n\n", datout); return(-1);}
      else  printf (" file %s opened\n",datout);

      for (lc=0; lc<anz->l; lc++)
      {
        if( (compare( lcase[lc].name, "NDTEMP", 6) == 6)||( compare( lcase[lc].name, "TEMP", 4) == 4)||( compare( lcase[lc].name, "TT3D", 4) == 4))
        {
          // check if a certain ds was requested, if yes scip all unwanted
          if((compare( dat[0], "ds", 2)== 2)&&(strlen(dat[0])>2)&&(atoi(&dat[0][2])!=lc+1)) continue;

          fprintf (handle2, "!\n");
          fprintf (handle2, "/OUTPUT,TERM\n");
          fprintf (handle2, "/SOLU\n");
          fprintf (handle2, "ANTYPE,0\n");
          fprintf (handle2, "EQSLV,FRONT,1,1,\n");
          fprintf (handle2, "!\n");
          fprintf (handle2, "! Temps LC%d\n", lc);
          fprintf (handle2, "ESEL,ALL \n");
          for (i=0; i<anz->n; i++)
          {
            fprintf (handle2, "BF, %6d, TEMP, %8.3f \n",  node[i].nr,
            lcase[lc].dat[0][node[i].nr]);
          }
          fprintf (handle2, "SOLVE\n");
          fprintf (handle2, "/OUTPUT,ansys,out,,APPEND\n");
          fprintf (handle2, "!\n");
          fprintf (handle2, "! PRINT LC %d\n", lc);
          fprintf (handle2, "/POST1\n");
          fprintf (handle2, "/PAGE, 24, 80, 100000, 132\n");
          fprintf (handle2, "esel, all \n");
          fprintf (handle2, "SET,LAST \n");
          fprintf (handle2, "prnsol, dof\n");
          fprintf (handle2, "prnsol, s, comp\n");
          fprintf (handle2, "! prnsol, bfe, temp\n");
          fprintf (handle2, "!\n");
        }
      }
      fclose(handle2);
    }
  }

  return (1);
}


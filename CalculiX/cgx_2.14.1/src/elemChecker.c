
/* ----------------------------------------------------------------  */
/* elemChecker korrigiert falsch orientierte elemente 24.06.97 wi    */
/* ACHTUNG! liefert jetzt einen pointer auf ein int-feld:            */
/* sum_korrelems,elemnr(1),elemnr(2)...                              */
/* ----------------------------------------------------------------  */

#include <extUtil.h>

int elemChecker(int sum_e, int *elems, Nodes *node, Elements *elem)
{
  int i, j, e;
  double v12[3], v13[3], v15[3], v15n[3], vn[3];
  double bv12,bv13,bv15, bvn, bv15n, bgrenz;
  int epuf[27];
  int e_korr=0;

  for (e=0; e<sum_e; e++)
  {
    i=elems[e];
    if( (elem[i].type == 1)||(elem[i].type == 4) )   /* HEXA */
    {
      v_result( &node[elem[i].nod[0]].nx, &node[elem[i].nod[1]].nx, v12);
      v_result( &node[elem[i].nod[0]].nx, &node[elem[i].nod[3]].nx, v13);
      v_result( &node[elem[i].nod[0]].nx, &node[elem[i].nod[4]].nx, v15);
      bv12=v_betrag(v12);
      bv13=v_betrag(v13);
      bv15=v_betrag(v15);
      if((bv12<1e-15)||(bv13<1e-15)||(bv15<1e-15))
      {
        v_result( &node[elem[i].nod[6]].nx, &node[elem[i].nod[5]].nx, v12);
        v_result( &node[elem[i].nod[6]].nx, &node[elem[i].nod[7]].nx, v13);
        v_result( &node[elem[i].nod[6]].nx, &node[elem[i].nod[2]].nx, v15);
      }
      bv15=v_betrag(v15);
      v_prod(v12,v13,vn);
      v_result(v15,vn,v15n);
      bvn=v_betrag(vn);
      bgrenz=sqrt(bvn*bvn+bv15*bv15);
      bv15n=v_betrag(v15n);
  
      // printf ("elemcheck:%d vn x=%e y=%e z=%e\n",elem[i].nr,vn[0],vn[1],vn[2]);
      if (bv15n > bgrenz)
      {
        // printf ("elem %d wrong defined, v15n=%e vgrenz=%e\n", elem[i].nr,bv15n,bgrenz);
        for (j=0; j<8; j++)
        {
          epuf[j] = elem[i].nod[j];
        }
        elem[i].nod[0] = epuf[4];
        elem[i].nod[1] = epuf[5];
        elem[i].nod[2] = epuf[6];
        elem[i].nod[3] = epuf[7];
        elem[i].nod[4] = epuf[0];
        elem[i].nod[5] = epuf[1];
        elem[i].nod[6] = epuf[2];
        elem[i].nod[7] = epuf[3];
        e_korr++;
        if (elem[i].type == 4)
        {
          for (j=0; j<4; j++)
          {
            epuf[j] = elem[i].nod[j+8];
            epuf[j+4] = elem[i].nod[j+16];
          }
          elem[i].nod[8] = epuf[4];
          elem[i].nod[9] = epuf[5];
          elem[i].nod[10] = epuf[6];
          elem[i].nod[11] = epuf[7];
          elem[i].nod[16] = epuf[0];
          elem[i].nod[17] = epuf[1];
          elem[i].nod[18] = epuf[2];
          elem[i].nod[19] = epuf[3];
        }

      }
    }

    else if( (elem[i].type == 2)||(elem[i].type == 5) )   /* PENTA */
    {
      v_result( &node[elem[i].nod[0]].nx, &node[elem[i].nod[1]].nx, v12);
      v_result( &node[elem[i].nod[0]].nx, &node[elem[i].nod[2]].nx, v13);
      v_result( &node[elem[i].nod[0]].nx, &node[elem[i].nod[3]].nx, v15);
      v_prod(v12,v13,vn);
      v_result(v15,vn,v15n);
      bvn=v_betrag(vn);
      bv15=v_betrag(v15);
      bgrenz=sqrt(bvn*bvn+bv15*bv15);
      bv15n=v_betrag(v15n);
  
      /* printf ("elemcheck:%d vn x=%e y=%e z=%e\n",elem[i].nr,vn[0],vn[1],vn[2]); */
      if (bv15n > bgrenz)
        {
        /* printf ("elem %d wrong defined, v15n=%e vgrenz=%e\n", elem[i].nr,bv15n,bgrenz);  */
        for (j=0; j<6; j++)
          {
          epuf[j] = elem[i].nod[j];
          }
        elem[i].nod[0] = epuf[3];
        elem[i].nod[1] = epuf[4];
        elem[i].nod[2] = epuf[5];
        elem[i].nod[3] = epuf[0];
        elem[i].nod[4] = epuf[1];
        elem[i].nod[5] = epuf[2];
        e_korr++;
        if (elem[i].type == 5)
        {
          for (j=0; j<3; j++)
          {
            epuf[j] = elem[i].nod[j+6];
            epuf[j+3] = elem[i].nod[j+12];
          }
          elem[i].nod[6] = epuf[3];
          elem[i].nod[7] = epuf[4];
          elem[i].nod[8] = epuf[5];
          elem[i].nod[12] = epuf[0];
          elem[i].nod[13] = epuf[1];
          elem[i].nod[14] = epuf[2];
        }

      }
    }

    else if( (elem[i].type == 3)||(elem[i].type == 6) )   /* TET */
    {
      v_result( &node[elem[i].nod[0]].nx, &node[elem[i].nod[1]].nx, v12);
      v_result( &node[elem[i].nod[0]].nx, &node[elem[i].nod[2]].nx, v13);
      v_result( &node[elem[i].nod[0]].nx, &node[elem[i].nod[3]].nx, v15);
      v_prod(v12,v13,vn);
      v_result(v15,vn,v15n);
      bvn=v_betrag(vn);
      bv15=v_betrag(v15);
      bgrenz=sqrt(bvn*bvn+bv15*bv15);
      bv15n=v_betrag(v15n);
  
      /* printf ("elemcheck:%d vn x=%e y=%e z=%e\n",elem[i].nr,vn[0],vn[1],vn[2]); */
      if (bv15n > bgrenz)
      {
        /* printf ("elem %d wrong defined, v15n=%e vgrenz=%e\n", elem[i].nr,bv15n,bgrenz);  */
        for (j=0; j<4; j++) epuf[j] = elem[i].nod[j];
        elem[i].nod[0] = epuf[1];
        elem[i].nod[1] = epuf[2];
        elem[i].nod[2] = epuf[3];
        elem[i].nod[3] = epuf[0];
        e_korr++;
        if (elem[i].type == 6)
        {
          for (j=4; j<10; j++) epuf[j] = elem[i].nod[j];
          elem[i].nod[4] = epuf[5];
          elem[i].nod[5] = epuf[9];
          elem[i].nod[6] = epuf[8];
          elem[i].nod[7] = epuf[4];
          elem[i].nod[8] = epuf[6];
          elem[i].nod[9] = epuf[7];
        }

      }
    }

  }
  return (e_korr);
}


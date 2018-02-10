/**********************************************************************
 *
 * TileLess
 *
 * TileLess is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * TileLess is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with TileLess.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright (C) 2016-2017 Nicklas Avén
 *
 **********************************************************************/


#include "theclient.h"
#include "interface/interface.h"
#include "mem.h"
#ifdef __ANDROID__
#include <jni.h>
#endif

/*void initialBBOX(GLfloat x, GLfloat y, GLfloat width, GLfloat *newBBOX)
{
    GLfloat ratio = (GLfloat) CURR_HEIGHT/ (GLfloat) CURR_WIDTH;
    GLfloat height = width * ratio;
    newBBOX[0] = x-width/2;
    newBBOX[1] = y-height/2;
    newBBOX[2] = x+width/2;
    newBBOX[3] = y+height/2;
}*/

int check_sql(char *sql)
{
    int i;
    int c;
    int to_check = ';';
    int sql_len = strlen(sql);
    for(i=0;i<sql_len;i++)
    {
        c=*sql;
        sql++;    
        if(c==to_check)
        {
            if(*sql)
            {                               
                sqlite3_close(projectDB);
                return EXIT_FAILURE;
            }
        }        
    }
    return 0;
}
void initialBBOX(GLfloat x, GLfloat y, GLfloat width, MATRIX *map_matrix)
{
    GLfloat ratio = (GLfloat) CURR_HEIGHT/ (GLfloat) CURR_WIDTH;
    GLfloat height = width * ratio;
    map_matrix->bbox[0] = x-width/2;
    map_matrix->bbox[1] = y-height/2;
    map_matrix->bbox[2] = x+width/2;
    map_matrix->bbox[3] = y+height/2;
}

int reset_matrix(MATRIX *matrix_handl)
{
    memset(matrix_handl->bbox, 0,4 *sizeof(GLfloat));

    memset(matrix_handl->matrix, 0, 16*sizeof(GLfloat));

    matrix_handl->vertical_enabled = 0;
    matrix_handl->zoom_enabled = 0;
    return 0;
}
void copy2ref_box(MATRIX *matrix_hndl,MATRIX *ref)
{
    ref->bbox[0] = matrix_hndl->bbox[0];
    ref->bbox[1] = matrix_hndl->bbox[1];
    ref->bbox[2] = matrix_hndl->bbox[2];
    ref->bbox[3] = matrix_hndl->bbox[3];
    return;
}

void windowResize(int newWidth,int newHeight,MATRIX *matrix_hndl,MATRIX *out)
{
    GLfloat meterPerPixel = (matrix_hndl->bbox[2] - matrix_hndl->bbox[0]) / (GLfloat) CURR_WIDTH;
    GLfloat oldCenterX = matrix_hndl->bbox[0] + (matrix_hndl->bbox[2] - matrix_hndl->bbox[0])/2;
    GLfloat oldCenterY = matrix_hndl->bbox[1] + (matrix_hndl->bbox[3] - matrix_hndl->bbox[1])/2;

    out->bbox[0] = oldCenterX - meterPerPixel * newWidth / 2;
    out->bbox[1] = oldCenterY - meterPerPixel * newHeight / 2;
    out->bbox[2] = oldCenterX + meterPerPixel * newWidth / 2;
    out->bbox[3] = oldCenterY + meterPerPixel * newHeight / 2;
    CURR_WIDTH = newWidth;
    CURR_HEIGHT = newHeight;

    return;
}



int multiply_matrices(GLfloat *matrix1,GLfloat *matrix2, GLfloat *theMatrix)
{
    int m1, m2, i;
    GLfloat  val=0; //m1o, m2o,

    memset(theMatrix, 0,sizeof(GLfloat)*16);
    for (i = 0; i<4; i++)
    {
        for (m2 = 0; m2<4; m2++)
        {
            val = 0;
            for (m1 = 0; m1<4; m1++)
            {
                val +=  matrix1[m2 + m1*4] * matrix2[m1 + 4 * i];
            }
            theMatrix[m2 + 4 * i] = val;
        }
    }
    return 0;
}

int multiply_matrix_vektor(GLfloat *matrix,GLfloat *vektor_in, GLfloat *vektor_out)
{
    int m1, m2;
    GLfloat  val=0; //m1o, m2o,

    memset(vektor_out, 0,sizeof(GLfloat)*4);

    for (m2 = 0; m2<4; m2++)
    {
        val = 0;
        for (m1 = 0; m1<4; m1++)
        {
            val +=  matrix[m2 + m1*4] * vektor_in[m1];
        }
        vektor_out[m2] = val;
    }
    return 0;
}


int px2m(GLfloat *bbox,GLint px_x,GLint px_y,GLfloat *w_x,GLfloat *w_y)
{

    log_this(10, "Entering get_bbox\n");

    GLfloat width = bbox[2]-bbox[0];
    GLfloat height = bbox[3]-bbox[1];
    *w_x = bbox[0] + px_x * (width/CURR_WIDTH);
    *w_y = bbox[3] - px_y * (height/CURR_HEIGHT);

    return 0;
}


int calc_translate(GLfloat w_x,GLfloat w_y, GLfloat *transl)
{


    log_this(10, "Entering calc_translate\n");

    memset(transl, 0,sizeof(float)*16);

    transl[0] = 1;
    transl[5] = 1;
    transl[10] = 1;
    transl[12] = -w_x;
    transl[13] = -w_y;
    transl[15] = 1;

    return 0;
}

int calc_scale(GLfloat *bbox, GLfloat zoom, GLfloat *scale)
{

    log_this(10, "Entering calc_scale\n");

    memset(scale, 0,sizeof(float)*16);

    GLfloat width = bbox[2]-bbox[0];
    GLfloat height = bbox[3]-bbox[1];

    scale[0] = (GLfloat) 2.0/width/zoom;
    scale[5] = (GLfloat) 2.0/height/zoom;
    scale[10] = (GLfloat) 1; //we set z-scaling 10 0.01 so we can give z-values from 0 to 100
    scale[15] = 1;
    return 0;
}

int search_string(const char *w,const char *s)
{
    int i, n;
    if(w==NULL ||s==NULL)
        return 0;
    int w_len = strlen(w);
    int s_len = strlen(s);

    for (i = 0; i<w_len; i++)
    {
        if(w[i] == s[0])
        {
            n = 1;
            while(w[i+n] == s[n])
            {
                n++;
            }
            if(n==s_len)
                return 1;
        }
    }
    return 0;
}




int multiply_short_array(GLshort *a, GLfloat v, GLshort ndims)
{
    int i;
    for(i=0; i<ndims; i++)
    {
        *(a+i) *= v;
    }
    return 0;
}


int multiply_float_array(float *a, float v, GLshort ndims)
{
    int i;
    for(i=0; i<ndims; i++)
    {
        *(a+i) *= v;
    }
    return 0;
}

float max_f(float a, float b)
{
    if (b > a)
        return b;
    else
        return a;
}
float min_f(float a, float b)
{
    if (b < a)
        return b;
    else
        return a;
}



#ifdef __ANDROID__
JNIEXPORT void JNICALL Java_no_jordogskog_app_gps_onNativeGPS(
    JNIEnv* env, jclass jcls,
    double latitude, double longitude, double acc)
{
    gps_in(latitude, longitude, acc);

}
#endif



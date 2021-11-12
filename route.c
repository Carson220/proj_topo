// 考虑单个时间片，计算交换机节点之间的路由

#include <stdio.h>
#include <string.h>

#define slot_num 44
#define fname_len 50
#define maxnum 66
#define maxdist 0x3f3f3f3f

int route[maxnum][maxnum]; // 记录松弛节点
int nextsw[maxnum]; // 记录下一跳交换机节点
int hop = 0;

// 用于输出路径，并写入文件
void out(int node1, int node2)
{
    if(route[node1][node2] == -1)
        return;
    out(node1, route[node1][node2]);
    nextsw[hop++] = route[node1][node2];
    // printf("%d ", route[node1][node2]);
    out(route[node1][node2], node2);
}

// 用于计算路由
int cal(int fnum)
{
    FILE *fp = NULL;
    FILE *fp1 = NULL;
    FILE *fp2 = NULL;
    FILE *fp3 = NULL;
    char fname[fname_len] = {0,};
    int node1, node2, num = 0;
    float dist = 0;
    int ctrl_num = 0; // 记录控制器数量
    int db_num = 0; // 记录数据库数量
    int matrix[maxnum][maxnum];
    int db_flag[maxnum]; // 标记数据库所在节点
    int db[maxnum] = {0,}; // 记录db序号
    int flag[maxnum] = {0,}; // 标记db已经被写入ctrl文件
    int mindist = 0;
    int minnode = -1;

    memset(matrix, 0x3f, sizeof(matrix));
    memset(route, -1, sizeof(route));
    memset(db_flag, 0, sizeof(db_flag));

    int i, j, k, m = 0;

    snprintf(fname, fname_len, "./switch_topo/switch_%d", fnum);
    if((fp=fopen(fname,"r"))==NULL)
    {
        printf("打开文件%s错误\n", fname);
        return -1;
    }
    fscanf(fp, "%d", &num);
    //printf("%d\n", num);

    while(!feof(fp))
    {
        fscanf(fp, "%d %d %f", &node1, &node2, &dist);
        //printf("%d %d %f\n", node1, node2, dist);
        matrix[node1][node2] = (int)(dist*1e5); // 把浮点数化作整数存储
        matrix[node2][node1] = (int)(dist*1e5);
        // matrix[node1][node2] = 1;
        // matrix[node2][node1] = 1;
    }
    fclose(fp);
    for(i = 0; i < num; i++)
    {
        matrix[i][i] = 0;
    }

    // Floyd 计算任意两点间距离
    for(k = 0; k < num; k++)
    {//从0开始遍历每一个中间节点，代表允许经过的结点编号<=k 
        for(i = 0; i < num; i++)
        {
            for(j = 0; j < num; j++)
            {
                if(matrix[i][k] == maxdist || matrix[k][j] == maxdist) 
                    continue;//中间节点不可达 
                if(matrix[i][j] == maxdist || matrix[i][k] + matrix[k][j] < matrix[i][j])//经过中间节点，路径变短 
                {
                    matrix[i][j] = matrix[i][k] + matrix[k][j];
                    route[i][j] = k;
                }
            }
        }
    }

    // for(i = 0; i < num; i++)
    // {
    //    for(j = 0; j < num; j++)
    //    {
    //       printf("matrix[%d][%d]=%d\n", i, j, matrix[i][j]);
    //    }
    // }

    // snprintf(fname, fname_len, "s2s_%d", fnum);
    // if((fp=fopen(fname,"w"))==NULL)
    // {
    //     printf("打开文件%s错误\n", fname);
    //     return -1;
    // }
    // for(i = 0; i < num; i++)
    // {
    //    fprintf(fp, "%d\n", i);
    //    for(j = 0; j < num; j++)
    //    {
    //         if(i != j)
    //         {
    //             // s2s route
    //             // printf("sw%d<->sw%d route: ", i, j);
    //             fprintf(fp, "%d %d ", i, j);
    //             hop = 0;
    //             nextsw[hop++] = i;
    //             // printf("%d ", i);
    //             out(i, j);
    //             nextsw[hop] = j;
    //             // printf("%d ", j);

    //             for(k = 0; k < hop; k++)
    //             {
    //                 // printf("sw%d-outport%d ", nextsw[k], nextsw[k+1]);
    //                 fprintf(fp, "%03d%03d ", nextsw[k], nextsw[k+1]);
    //             }
    //             fprintf(fp, "\n");
    //         }
    //    }
    // }
    // fclose(fp);

    // snprintf(fname, fname_len, "ctrl_%d", fnum);
    // if((fp1=fopen(fname,"r"))==NULL)
    // {
    //     printf("打开文件%s错误\n", fname);
    //     return -1;
    // }
    // snprintf(fname, fname_len, "c2s_%d", fnum);
    // if((fp2=fopen(fname,"w"))==NULL)
    // {
    //     printf("打开文件%s错误\n", fname);
    //     return -1;
    // }

    // fscanf(fp1, "%d\n", &ctrl_num);
    // for(k = 0; k < ctrl_num; k++)
    // {
    //     fscanf(fp1, "%d\n", &i);
    //     do{
    //         fscanf(fp1, "%d", &j);
    //         if(i != j)
    //         {
    //             // c2s route
    //             fprintf(fp2, "%d %d ", i, j);
    //             hop = 0;
    //             nextsw[hop++] = i;
    //             out(i, j);
    //             nextsw[hop] = j;

    //             for(m = 0; m < hop; m++)
    //             {
    //                 fprintf(fp2, "%03d%03d ", nextsw[m], nextsw[m+1]);
    //             }
    //             fprintf(fp2, "\n");

    //             // s2c route
    //             fprintf(fp2, "%d %d ", j, i);
    //             hop = 0;
    //             nextsw[hop++] = j;
    //             out(j, i);
    //             nextsw[hop] = i;

    //             for(m = 0; m < hop; m++)
    //             {
    //                 fprintf(fp2, "%03d%03d ", nextsw[m], nextsw[m+1]);
    //             }
    //             fprintf(fp2, "\n");
    //         }
    //     }while(fgetc(fp1) == ' ');
    // }
    // fclose(fp1);

    // // standby_ctrl <-> sw
    // snprintf(fname, fname_len, "standby_ctrl_%d", fnum);
    // if((fp=fopen(fname,"r"))==NULL)
    // {
    //     printf("打开文件%s错误\n", fname);
    //     return -1;
    // }
    // fscanf(fp, "%d\n", &num);
    // for(j = 0; j < num; j++)
    // {
    //     fscanf(fp, "%d", &i);
    //     if(i != j)
    //     {
    //         // c2s route
    //         fprintf(fp2, "%d %d ", i, j);
    //         hop = 0;
    //         nextsw[hop++] = i;
    //         out(i, j);
    //         nextsw[hop] = j;

    //         for(m = 0; m < hop; m++)
    //         {
    //             fprintf(fp2, "%03d%03d ", nextsw[m], nextsw[m+1]);
    //         }
    //         fprintf(fp2, "\n");

    //         // s2c route
    //         fprintf(fp2, "%d %d ", j, i);
    //         hop = 0;
    //         nextsw[hop++] = j;
    //         out(j, i);
    //         nextsw[hop] = i;

    //         for(m = 0; m < hop; m++)
    //         {
    //             fprintf(fp2, "%03d%03d ", nextsw[m], nextsw[m+1]);
    //         }
    //         fprintf(fp2, "\n");
    //     }
    // }
    // fclose(fp2);
    // fclose(fp);

    snprintf(fname, fname_len, "db");
    if((fp=fopen(fname,"r"))==NULL)
    {
        printf("打开文件%s错误\n", fname);
        return -1;
    }
    fscanf(fp, "%d", &db_num);
    snprintf(fname, fname_len, "./db_conn_ctrl/db_%d", fnum);
    if((fp1=fopen(fname,"r"))==NULL)
    {
        printf("打开文件%s错误\n", fname);
        return -1;
    }
    snprintf(fname, fname_len, "./route_c2d/c2d_%d", fnum);
    if((fp2=fopen(fname,"w"))==NULL)
    {
        printf("打开文件%s错误\n", fname);
        return -1;
    }
    for(k = 0; k < db_num; k++)
    {
        fscanf(fp1, "%d", &i);
        fgetc(fp1); // read '\n'
        while(fgetc(fp1) == ' ')
        {
            fscanf(fp1, "%d", &j);

            // slot_k ctrl_j conf
            // snprintf(fname, fname_len, "slot_%d_ctrl_%d", fnum, j);
            // if((fp3=fopen(fname,"w"))==NULL)
            // {
            //     printf("打开文件%s错误\n", fname);
            //     return -1;
            // }
            // fprintf(fp3, "%d\n%d\n%d\n", fnum, j+1, i+1);
            // fclose(fp3);
            // printf("slot = %d, ctrl = %d, db = %d\n", fnum, j, i);

            // if(i != j)
            // {
            //     // d2c route
            //     fprintf(fp2, "%d %d ", i, j);
            //     hop = 0;
            //     nextsw[hop++] = i;
            //     out(i, j);
            //     nextsw[hop] = j;

            //     for(m = 0; m < hop; m++)
            //     {
            //         fprintf(fp2, "%03d%03d ", nextsw[m], nextsw[m+1]);
            //     }
            //     fprintf(fp2, "\n");

            //     // c2d route
            //     fprintf(fp2, "%d %d ", j, i);
            //     hop = 0;
            //     nextsw[hop++] = j;
            //     out(j, i);
            //     nextsw[hop] = i;

            //     for(m = 0; m < hop; m++)
            //     {
            //         fprintf(fp2, "%03d%03d ", nextsw[m], nextsw[m+1]);
            //     }
            //     fprintf(fp2, "\n");
            // }
        }

        for(j = 0; j < num; j++)
        {
            if(i != j)
            {
                // d2c route
                fprintf(fp2, "%d %d ", i, j);
                hop = 0;
                nextsw[hop++] = i;
                out(i, j);
                nextsw[hop] = j;

                for(m = 0; m < hop; m++)
                {
                    fprintf(fp2, "%03d%03d ", nextsw[m], nextsw[m+1]);
                }
                fprintf(fp2, "\n");

                // c2d route
                fprintf(fp2, "%d %d ", j, i);
                hop = 0;
                nextsw[hop++] = j;
                out(j, i);
                nextsw[hop] = i;

                for(m = 0; m < hop; m++)
                {
                    fprintf(fp2, "%03d%03d ", nextsw[m], nextsw[m+1]);
                }
                fprintf(fp2, "\n");
            }
        }
    }
    fclose(fp1);
    fclose(fp2);

    fscanf(fp, "%d", &i);
    j = 0;
    while(fscanf(fp, "%d", &i)  == 1)
    {
        db_flag[i] = 1;
        db[j++] = i;
    }
    fclose(fp);
    snprintf(fname, fname_len, "./route_d2d/d2d_%d", fnum);
    if((fp1=fopen(fname,"w"))==NULL)
    {
        printf("打开文件%s错误\n", fname);
        return -1;
    }
    for(i = 0; i < num; i++)
    {
       if(db_flag[i] == 1)
       {
           fprintf(fp1, "%d\n", i);
           for(j = 0; j < num; j++)
           {
               if(db_flag[j] == 1 && i != j)
               {
                    // d2d route
                    // printf("db%d<->db%d route: ", i, j);
                    fprintf(fp1, "%d %d ", i, j);
                    hop = 0;
                    nextsw[hop++] = i;
                    // printf("%d ", i);
                    out(i, j);
                    nextsw[hop] = j;
                    // printf("%d ", j);

                    for(k = 0; k < hop; k++)
                    {
                        // printf("sw%d-outport%d ", nextsw[k], nextsw[k+1]);
                        fprintf(fp1, "%03d%03d ", nextsw[k], nextsw[k+1]);
                    }
                    fprintf(fp1, "\n");
               }
           }
       }
    }
    fclose(fp1);

    // ctrl_fnum conf
    for(i = 0; i < num; i++)
    {
        memset(flag, 0, sizeof(flag));
        snprintf(fname, fname_len, "./ctrl_conn_db/ctrl_%d", i);
        if((fp=fopen(fname,"a+"))==NULL)
        {
            printf("打开文件%s错误\n", fname);
            return -1;
        }

        if(fnum == 0) fprintf(fp, "%d\n", i);
        for(j = 0; j < db_num; j++)
        {
            mindist = maxdist;
            for(k = 0; k < db_num; k++)
            {
                if(matrix[i][k] < mindist && flag[db[k]] != 1)
                {
                    mindist = matrix[i][k];
                    minnode = k;
                }
            }
            flag[db[minnode]] = 1;
            fprintf(fp, "%d ", db[minnode]);
        }
        fprintf(fp, "\n");

        fclose(fp);
    }

    return 0;
}

int main()
{
    int fnum = 0;
    for(fnum = 0; fnum < slot_num; fnum++)
    {
        printf("test_%d route 默认路由计算完毕\n", fnum);
        cal(fnum);
    }
    return 0;
}

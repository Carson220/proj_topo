// 考虑单个时间片，利用Suurballe's algorithm计算DB节点之间的路由

#include <stdio.h>
#include <string.h>

#define slot_num 44
#define fname_len 50
#define maxnum 66
#define maxdist 0x3f3f3f3f

// 用于计算路由
int cal(int fnum, int src, int dst)
{
    FILE *fp = NULL;
    FILE *fp1 = NULL;
    char fname[fname_len] = {0,};
    int node1, node2, num = 0;
    float dist = 0;
    int matrix[maxnum][maxnum];
    int mindist = 0;
    int minnode = -1;
    int curnode = -1;
    int prenode = -1;
    int node[maxnum][4] = {0,}; // node[x][0]标记该节点是否已经加入最短路, node[x][1]表示到源节点到该节点的最短距离, node[x][2]记录该节点在最短路径上的前驱节点, node[x][4]记录该节点在最短路径上的后继节点
    int node_new[maxnum][4] = {0,}; // 用于第二次dijkstra计算
    int path[maxnum] = {0,}; // 第一次dijkstra的结果
    int path_new[maxnum] = {0,}; // 第二次dijkstra的结果
    int path_1[maxnum] = {0,}; // 链路分离路径1
    int path_2[maxnum] = {0,}; // 链路分离路径2
    int flag = 0; // flag=1表示第一次dijkstra计算成功

    memset(matrix, 0x3f, sizeof(matrix));
    memset(path, -1, sizeof(path));
    memset(path_new, -1, sizeof(path_new));
    memset(path_1, -1, sizeof(path_1));
    memset(path_2, -1, sizeof(path_2));

    int i, j, k = 0;
    for(i = 0; i < maxnum; i++)
    {
        node[i][0] = 0;
        node[i][1] = maxdist;
        node[i][2] = -1;
        node[i][3] = -1;
        node_new[i][0] = 0;
        node_new[i][1] = maxdist;
        node_new[i][2] = -1;
        node_new[i][3] = -1;
    }

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
    }
    fclose(fp);
    for(i = 0; i < num; i++)
    {
        matrix[i][i] = 0;
    }

    // Dijkstra 计算两点间距离
    node[src][0] = 1;
    node[src][1] = 0;
    for(i = 0; i < maxnum; i++)
    {
        if(node[i][0] == 0 && matrix[src][i] < maxdist)
        {
            node[i][1] = matrix[src][i];
            node[i][2] = src;
        }
    }

    for(i = 0; i < maxnum; i++)
    {
        mindist = maxdist;
        minnode = -1;
        for(j = 0; j < maxnum; j++)
        {
            if(node[j][0] != 0) continue;
            if(node[j][1] < mindist)
            {
                mindist = node[j][1];
                minnode = j;
            }
        }

        if(minnode == -1) break;
        if(minnode == dst) flag = 1;

        node[minnode][0] = 1;
        for(j = 0; j < maxnum; j++)
        {
            if(node[j][0] == 0 && node[j][1] > matrix[minnode][j] + node[minnode][1] && matrix[minnode][j] < maxdist)
            {
                node[j][1] = matrix[minnode][j] + node[minnode][1];
                node[j][2] = minnode;
            }
        }
    }

    // 生成残差图
    if(flag == 1)
    {
        for(i = 0; i < maxnum; i++)
        {
            for(j = 0; j < maxnum; j++)
            {
                matrix[i][j] = matrix[i][j] + node[i][1] - node[j][1];
            }
        }
        
        curnode = dst;
        prenode = node[curnode][2];
        k = 0;
        path[k++] = curnode;
        while(prenode != -1)
        {
            node[prenode][3] = curnode;
            matrix[curnode][prenode] = 0;
            matrix[prenode][curnode] = maxdist;
            curnode = prenode;
            prenode = node[curnode][2];
            path[k++] = curnode;
        }

        // printf("db%d - db%d: \n", src, dst);
        // k = 0;
        // printf("\tpath: ");
        // while(path[k] != -1)
        // {
        //     printf("%d ", path[k++]);
        // }
        // printf("\n");
        
        // 第二次Dijkstra计算
        node_new[src][0] = 1;
        node_new[src][1] = 0;
        for(i = 0; i < maxnum; i++)
        {
            if(node_new[i][0] == 0 && matrix[src][i] < maxdist)
            {
                node_new[i][1] = matrix[src][i];
                node_new[i][2] = src;
            }
        }

        for(i = 0; i < maxnum; i++)
        {
            mindist = maxdist;
            minnode = -1;
            for(j = 0; j < maxnum; j++)
            {
                if(node_new[j][0] != 0) continue;
                if(node_new[j][1] < mindist)
                {
                    mindist = node_new[j][1];
                    minnode = j;
                }
            }

            if(minnode == -1) break;
            if(minnode == dst) break;

            node_new[minnode][0] = 1;
            for(j = 0; j < maxnum; j++)
            {
                if(node_new[j][0] == 0 && node_new[j][1] > matrix[minnode][j] + node_new[minnode][1] && matrix[minnode][j] < maxdist)
                {
                    node_new[j][1] = matrix[minnode][j] + node_new[minnode][1];
                    node_new[j][2] = minnode;
                }
            }
        }

        if(minnode == dst)
        {
            curnode = dst;
            prenode = node_new[curnode][2];
            k = 0;
            path_new[k++] = curnode;
            while(prenode != -1)
            {
                node_new[prenode][3] = curnode;
                curnode = prenode;
                prenode = node_new[curnode][2];
                path_new[k++] = curnode;
            }

            // k = 0;
            // printf("\tpath_new: ");
            // while(path_new[k] != -1)
            // {
            //     printf("%d ", path_new[k++]);
            // }
            // printf("\n");

            // 比较两条路径，删除重复部分
            i = 0;
            while(path[i+1] != src)
            {
                j = 0;
                while(path_new[j+1] != src)
                {
                    if(path_new[j] == path[i+1] && path_new[j+1] == path[i])
                    {
                        node[path[i+1]][3] = -1;
                        node_new[path[i]][3] = -1;
                        // printf("del overlap link sw%d - sw%d\n", path[i+1], path[i]);
                    }
                    j++;
                }
                i++;
            }

            // 重组路径
            path_1[0] = src;
            path_1[1] = node[src][3];
            curnode = path_1[1];
            k = 2;
            while(curnode != dst)
            {
                if(node[curnode][3] == -1)
                {
                    path_1[k] = node_new[curnode][3];
                    node_new[curnode][3] = -1;
                }
                else
                {
                    path_1[k] = node[curnode][3];
                    node[curnode][3] = -1;
                }    
                curnode = path_1[k];
                k++;
            }
            path_2[0] = src;
            path_2[1] = node_new[src][3];
            curnode = path_2[1];
            k = 2;
            while(curnode != dst)
            {
                if(node[curnode][3] == -1)
                    path_2[k] = node_new[curnode][3];
                else
                    path_2[k] = node[curnode][3];
                curnode = path_2[k];
                k++;
            }

            // printf("db%d - db%d: \n", src, dst);
            // k = 0;
            // printf("\tpath_1: ");
            // while(path_1[k] != -1)
            // {
            //     printf("%d ", path_1[k++]);
            // }
            // printf("\n");
            // k = 0;
            // printf("\tpath_2: ");
            // while(path_2[k] != -1)
            // {
            //     printf("%d ", path_2[k++]);
            // }
            // printf("\n\n");
        }
        else
        {
            printf("dijkstra_2 failed\n");
        }
    }
    else
    {
        printf("dijkstra_1 failed\n");
    }

    // d2d route
    snprintf(fname, fname_len, "./route_d2d/d2d_%d", fnum);
    if((fp1=fopen(fname,"a"))==NULL)
    {
        printf("打开文件%s错误\n", fname);
        return -1;
    }

    fprintf(fp1, "%d %d ", src, dst);
    k = 0;
    while(path_1[k+1] != -1)
    {
        fprintf(fp1, "%03d%03d ", path_1[k], path_1[k+1]);
        k++;
    }
    fprintf(fp1, "\n");
    fprintf(fp1, "%d %d ", src, dst);
    k = 0;
    while(path_2[k+1] != -1)
    {
        fprintf(fp1, "%03d%03d ", path_2[k], path_2[k+1]);
        k++;
    }
    fprintf(fp1, "\n");  

    fclose(fp1);
    return 0;
}

int main()
{
    int fnum = 0;
    int db[] = {13, 16, 31, 46, 50, 54};
    int db_num = 6;
    int i, j = 0;
    FILE *fp = NULL;
    char fname[fname_len] = {0,};

    for(fnum = 0; fnum < slot_num; fnum++)
    {
        printf("test_%d route_d2d 默认路由计算\n", fnum);
        for(i = 0; i < db_num; i++)
        {
            snprintf(fname, fname_len, "./route_d2d/d2d_%d", fnum);
            if((fp=fopen(fname,"a"))==NULL)
            {
                printf("打开文件%s错误\n", fname);
                return -1;
            }
            fprintf(fp, "%d\n", db[i]);
            fclose(fp);

            for(j = 0; j < db_num; j++)
            {
                if(i != j)
                {
                    cal(fnum, db[i], db[j]);
                }
            }
        }
    }
    return 0;
}

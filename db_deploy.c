// 考虑全部时间片，基于p-中值模型按距离进行数据库选址（总距离最短）

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h> // 使用当前时钟做种子
#include <unistd.h> // sleep()
// #define K 6 // database num
#define slot_num 44
#define fname_len 20
#define maxnum 66
#define maxdist 0x3f3f3f3f
#define MAX 1024 // 从文件中读取的数据最大长度

// 用于交叉验证选取合适的K值
int cv(int db_num)
{
    int K = db_num;
    FILE *fp = NULL;
    char fname[fname_len] = {0,};
    int node1, node2, num = 0;
    float dist = 0;
    int dist_temp = 0;
    int mindist[slot_num][K+1]; // 记录每个簇的最短距离和的最小值
    int dist_sum = 2e9; // mindist求和，评估聚类效果
    int optval = 2e9; // 记录每次聚类的优化值

    int map[slot_num][maxnum]; // 记录每个点所属的簇 map[0~65] = 1~k
    int center[slot_num][K+1]; // 记录每个簇的中心点 center[1~k] = 0~65
    int center_temp[slot_num][K+1]; // 记录每个簇的临时中心点
    int center_flag[slot_num][maxnum]; // 标记每个中心点 center_flag[0~65] = 0/1
    int matrix[slot_num][maxnum][maxnum];

    int alter_location[maxnum]; // 标记备选中心节点 alter_location[0~65] = 0/1
    int alter_node[maxnum+1]; // 记录备选中心节点 alter_node[1~66] = 0~65

    int ctrl_flag[maxnum]; // 标记控制器部署位置 ctrl_flag[0~65] = 0/1
    int ctrl_num = 0; // 记录控制器数量

    int i, j, k, m, n = 0;
    int alter_num = 0; // 记录备选中心节点个数
    int center_num = 0; // 记录中心节点个数
    char buffer[MAX] = {0, }; // 存储从文件中读取的数据

    int cost[slot_num][maxnum][maxnum+1]; // 记录对于时间片m节点i，删除备选中心节点k之后增加的代价 cost[m][i][k]
    int funval[maxnum+1]; // 记录删除备选中心节点k之后增加的总代价 funval[k]
    int minval = maxdist;
    int minnode = 0;
    int flag[maxnum]; // 0-exist 1-deleted

    memset(mindist, 0x3f, sizeof(mindist));
    memset(map, 0, sizeof(map));
    memset(center, 0, sizeof(center));
    memset(center_temp, 0, sizeof(center_temp));
    memset(center_flag, 0, sizeof(center_flag));
    memset(matrix, 0x3f, sizeof(matrix));
    memset(alter_location, 0, sizeof(alter_location));
    memset(alter_node, 0, sizeof(alter_node));
    memset(ctrl_flag, 0, sizeof(ctrl_flag));
    memset(cost, 0, sizeof(cost));
    memset(funval, 0, sizeof(funval));
    memset(flag, 0, sizeof(flag));

    // 选取数据库部署备选位置
    for(m = 0; m < slot_num; m++)
    {
        dist_sum = 2e9;
        optval = 2e9;

        snprintf(fname, fname_len, "test_%d", m);
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
            matrix[m][node1][node2] = (int)(dist*1e5); // 把浮点数化作整数存储
            matrix[m][node2][node1] = (int)(dist*1e5);
        }
        fclose(fp);
        for(i = 0; i < num; i++)
        {
            matrix[m][i][i] = 0;
        }

        // Floyd 计算任意两点间距离
        for(k = 0; k < num; k++)
        {// 从0开始遍历每一个中间节点，代表允许经过的结点编号<=k 
            for(i = 0; i < num; i++)
            {
                for(j = 0; j < num; j++)
                {
                    if(matrix[m][i][k] == maxdist || matrix[m][k][j] == maxdist) 
                        continue; // 中间节点不可达 
                    if(matrix[m][i][j] == maxdist || matrix[m][i][k] + matrix[m][k][j] < matrix[m][i][j])//经过中间节点，路径变短 
                        matrix[m][i][j] = matrix[m][i][k] + matrix[m][k][j];
                }
            }
        }

        // for(i = 0; i < num; i++)
        // {
        //    for(j = 0; j < num; j++)
        //    {
        //       printf("matrix[%d][%d][%d]=%d\n", m, i, j, matrix[m][i][j]);
        //    }
        // }

        // 读取控制器部署信息
        // snprintf(fname, fname_len, "ctrl_%d", m);
        // if((fp=fopen(fname,"r"))==NULL)
        // {
        //     printf("打开文件%s错误\n", fname);
        //     return -1;
        // }
        // fscanf(fp, "%d", &ctrl_num[m]);
        // printf("ctrl_num[%d]=%d\n", m, ctrl_num[m]);
        snprintf(fname, fname_len, "ctrl");
        if((fp=fopen(fname,"r"))==NULL)
        {
            printf("打开文件%s错误\n", fname);
            return -1;
        }
        fscanf(fp, "%d", &ctrl_num);

        k = 1;
        while(!feof(fp))
        {
            fscanf(fp, "%d", &i);
            // printf("ctrl_%d=%d\n", i, i);
            ctrl_flag[i] = 1;
            // fgets(buffer , MAX , fp);
        }
        // printf("\n");
        fclose(fp);
 
        // K-means 聚类
        // 随机选取中心点
        srand((unsigned)time(NULL)); 
        center_num = K;
        
        for(; k <= center_num; k++)
        {
            i = rand() % num;
            while(center_flag[m][i] == 1)
            {
                i = rand() % num;
            }
            center[m][k] = i;
            center_temp[m][k] = i;
            center_flag[m][i] = 1;
            // printf("center[%d][%d] = %d\n", m, k, center[m][k]);
        }
        
        while(optval > 1e3)
        {
            // 聚类
            for(i = 0; i < num; i++)
            {
                dist_temp = maxdist; // 记录每个点到各个中心点的最小距离
                for(k = 1; k <= center_num; k++)
                {
                    if(matrix[m][i][center[m][k]] < dist_temp)
                    {
                        dist_temp = matrix[m][i][center[m][k]];
                        map[m][i] = k;
                        // printf("map[%d][%d] = %d\n", m, i, k);
                    }
                }
            }

            // 重新选取中心点
            for(i = 0; i < num; i++)
            {
                dist_temp = 0; // 记录到同一个簇其余（控制器）点的最短距离之和
                for(j = 0; j < num; j++)
                {
                    if(map[m][i] == map[m][j] && ctrl_flag[j] == 1)
                    {
                        dist_temp += matrix[m][i][j];
                    }
                }
                if(dist_temp < mindist[m][map[m][i]])
                {
                    mindist[m][map[m][i]] = dist_temp;
                    center_temp[m][map[m][i]] = i;
                    // printf("center_temp[%d][%d] = %d\n", m, map[m][i], i);
                }
            }

            dist_temp = 0; // mindist求和，评估聚类效果
            for(k = 1; k <= center_num; k++)
            {
                center[m][k] = center_temp[m][k];
                dist_temp += mindist[m][k];
                // printf("center[%d][%d] = %d\n", m, k, center[m][k]);
            }
            optval = dist_sum - dist_temp;
            dist_sum = dist_temp;
            // printf("test_%d optval = %d\n", m, optval);
            // printf("test_%d dist_sum = %d\n", m, dist_sum);
        }
        // printf("\n");
        for(k = 1; k <= center_num; k++)
        {
            alter_location[center[m][k]] = 1; // 标记备选节点
        }
    }

    // 利用贪婪取走启发式算法(Greedy Dropping Heuristic Algorithm)，删减备选位置
    alter_num = 0; // 记录备选中心节点个数
    // printf("备选中心节点：");
    for(i = 0; i < num; i++)
    {
        if(alter_location[i] == 1)
        {
            alter_num++;
            alter_node[alter_num] = i; // 记录备选中心节点
            // printf("%d ", i);
        }
    }
    // printf("\n");

    // 聚类将每个节点分配给最近的备选中心节点
    dist_sum = 0; // 记录所有时间片的总距离
    for(m = 0; m < slot_num; m++)
    {
        for(i = 0; i < num; i++)
        {
            dist_temp = maxdist; // 记录每个点到各个备选中心点的最小距离
            for(k = 1; k <= alter_num; k++)
            {
                if(matrix[m][i][alter_node[k]] < dist_temp)
                {
                    dist_temp = matrix[m][i][alter_node[k]];
                    map[m][i] = k;
                    // printf("map[%d][%d] = %d\n", m, i, k);
                }
            }
            if(ctrl_flag[i] == 1)
                dist_sum += dist_temp;
        }
    }

    // 循环删除备选中心节点，直到中心节点个数为K
    // printf("alter_num=%d, K=%d\n", alter_num, K);
    for(n = alter_num; n > K; n--) 
    {
        memset(cost, 0, sizeof(cost));
        memset(funval, 0, sizeof(funval));
        for(k = 1; k <= alter_num; k++)// 遍历备选中心节点，计算删除之后的目标函数值
        {
            for(m = 0; m < slot_num; m++)
            {
                for(i = 0; i < num; i++)
                {
                    if(map[m][i] == k && ctrl_flag[i] == 1)
                    {
                        cost[m][i][k] -= matrix[m][i][alter_node[k]];
                        // printf("matrix[%d][%d][alter_node[%d](%d)] = %d\n", m, i, k, alter_node[k], matrix[m][i][alter_node[k]]);
                        dist_temp = maxdist; // 记录到各个中心点的最小距离
                        for(j = 1; j <= alter_num; j++)
                        {
                            if(matrix[m][i][alter_node[j]] < dist_temp && j != k && flag[j] != 1)
                            {
                                dist_temp = matrix[m][i][alter_node[j]];
                                // map[m][i] = k;
                                // printf("map[%d][%d] = %d\n", m, i, k);
                                // printf("center = alter_node[%d] = %d\n", j, alter_node[j]);
                            }
                        }
                        cost[m][i][k] += dist_temp;
                        // printf("dist_temp = %d\n", dist_temp);
                        // printf("cost[%d][%d][%d] = %d\n", m, i, k, cost[m][i][k]);
                        funval[k] += cost[m][i][k];
                        // printf("funval[%d] = %d\n", k, funval[k]);
                        // printf("\n");
                    }
                }
            }
        }
        // 选取代价最小的节点删除
        minval = maxdist;
        minnode = 0;
        for(k = 1; k <= alter_num; k++)
        {
            if(funval[k] < minval && flag[k] != 1)
            {
                minval = funval[k];
                minnode = k;
            }
        }
        dist_sum += minval;
        flag[minnode] = 1;
        // printf("del %d\n", alter_node[minnode]);
        for(m = 0; m < slot_num; m++)
        {
            for(i = 0; i < num; i++)
            {
                if(map[m][i] == minnode && ctrl_flag[i] == 1)
                {
                    dist_temp = maxdist; // 记录到各个中心点的最小距离
                    k = 0;
                    for(j = 1; j <= alter_num; j++)
                    {
                        if(matrix[m][i][alter_node[j]] < dist_temp && flag[j] != 1)
                        {
                            dist_temp = matrix[m][i][alter_node[j]];
                            k = j;
                            // printf("map[%d][%d] = %d\n", m, i, minnode);
                            // printf("center = alter_node[%d] = %d\n", j, alter_node[j]);
                        }
                    }
                    map[m][i] = k;
                }
            }
        }
    }

    // printf("中心节点：");
    // for(k = 1; k <= alter_num; k++)
    // {
    //     if(flag[k] != 1)
    //         printf("%d ",alter_node[k]);
    // }
    // printf("\n");
    // printf("dist_sum = %d\n", dist_sum);

    return dist_sum;
}

// 用于根据给定的K值计算数据库部署方案，并写入文件
int cal(int db_num)
{
    int K = db_num;
    FILE *fp = NULL;
    char fname[fname_len] = {0,};
    int fnum = 0;
    int node1, node2, num = 0;
    float dist = 0;
    int dist_temp = 0;
    int mindist[slot_num][K+1]; // 记录每个簇的最短距离和的最小值
    int dist_sum = 2e9; // mindist求和，评估聚类效果
    int optval = 2e9; // 记录每次聚类的优化值

    int map[slot_num][maxnum]; // 记录每个点所属的簇 map[0~65] = 1~k
    int center[slot_num][K+1]; // 记录每个簇的中心点 center[1~k] = 0~65
    int center_temp[slot_num][K+1]; // 记录每个簇的临时中心点
    int center_flag[slot_num][maxnum]; // 标记每个中心点 center_flag[0~65] = 0/1
    int matrix[slot_num][maxnum][maxnum];

    int alter_location[maxnum]; // 标记备选中心节点 alter_location[0~65] = 0/1
    int alter_node[maxnum+1]; // 记录备选中心节点 alter_node[1~66] = 0~65

    int ctrl_flag[maxnum]; // 标记控制器部署位置 ctrl_flag[0~65] = 0/1
    int ctrl_num = 0; // 记录控制器数量

    int i, j, k, m, n = 0;
    int alter_num = 0; // 记录备选中心节点个数
    int center_num = 0; // 记录中心节点个数
    char buffer[MAX] = {0, }; // 存储从文件中读取的数据

    int cost[slot_num][maxnum][maxnum+1]; // 记录对于时间片m节点i，删除备选中心节点k之后增加的代价 cost[m][i][k]
    int funval[maxnum+1]; // 记录删除备选中心节点k之后增加的总代价 funval[k]
    int minval = maxdist;
    int minnode = 0;
    int flag[maxnum]; // 0-exist 1-deleted

    memset(mindist, 0x3f, sizeof(mindist));
    memset(map, 0, sizeof(map));
    memset(center, 0, sizeof(center));
    memset(center_temp, 0, sizeof(center_temp));
    memset(center_flag, 0, sizeof(center_flag));
    memset(matrix, 0x3f, sizeof(matrix));
    memset(alter_location, 0, sizeof(alter_location));
    memset(alter_node, 0, sizeof(alter_node));
    memset(ctrl_flag, 0, sizeof(ctrl_flag));
    memset(cost, 0, sizeof(cost));
    memset(funval, 0, sizeof(funval));
    memset(flag, 0, sizeof(flag));

    // 选取数据库部署备选位置
    for(m = 0; m < slot_num; m++)
    {
        dist_sum = 2e9;
        optval = 2e9;

        snprintf(fname, fname_len, "test_%d", m);
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
            matrix[m][node1][node2] = (int)(dist*1e5); // 把浮点数化作整数存储
            matrix[m][node2][node1] = (int)(dist*1e5);
        }
        fclose(fp);
        for(i = 0; i < num; i++)
        {
            matrix[m][i][i] = 0;
        }

        // Floyd 计算任意两点间距离
        for(k = 0; k < num; k++)
        {// 从0开始遍历每一个中间节点，代表允许经过的结点编号<=k 
            for(i = 0; i < num; i++)
            {
                for(j = 0; j < num; j++)
                {
                    if(matrix[m][i][k] == maxdist || matrix[m][k][j] == maxdist) 
                        continue; // 中间节点不可达 
                    if(matrix[m][i][j] == maxdist || matrix[m][i][k] + matrix[m][k][j] < matrix[m][i][j])//经过中间节点，路径变短 
                        matrix[m][i][j] = matrix[m][i][k] + matrix[m][k][j];
                }
            }
        }

        // for(i = 0; i < num; i++)
        // {
        //    for(j = 0; j < num; j++)
        //    {
        //       printf("matrix[%d][%d][%d]=%d\n", m, i, j, matrix[m][i][j]);
        //    }
        // }

        // 读取控制器部署信息
        // snprintf(fname, fname_len, "ctrl_%d", m);
        // if((fp=fopen(fname,"r"))==NULL)
        // {
        //     printf("打开文件%s错误\n", fname);
        //     return -1;
        // }
        // fscanf(fp, "%d", &ctrl_num[m]);
        // printf("ctrl_num[%d]=%d\n", m, ctrl_num[m]);
        snprintf(fname, fname_len, "ctrl");
        if((fp=fopen(fname,"r"))==NULL)
        {
            printf("打开文件%s错误\n", fname);
            return -1;
        }
        fscanf(fp, "%d", &ctrl_num);

        k = 1;
        while(!feof(fp))
        {
            fscanf(fp, "%d", &i);
            // printf("ctrl_%d=%d\n", i, i);
            ctrl_flag[i] = 1;
            // fgets(buffer , MAX , fp);
        }
        // printf("\n");
        fclose(fp);
 
        // K-means 聚类
        // 随机选取中心点
        srand((unsigned)time(NULL)); 
        center_num = K;

        for(; k <= center_num; k++)
        {
            i = rand() % num;
            while(center_flag[m][i] == 1)
            {
                i = rand() % num;
            }
            center[m][k] = i;
            center_temp[m][k] = i;
            center_flag[m][i] = 1;
            // printf("center[%d][%d] = %d\n", m, k, center[m][k]);
        }
        
        while(optval > 1e3)
        {
            // 聚类
            for(i = 0; i < num; i++)
            {
                dist_temp = maxdist; // 记录每个点到各个中心点的最小距离
                for(k = 1; k <= center_num; k++)
                {
                    if(matrix[m][i][center[m][k]] < dist_temp)
                    {
                        dist_temp = matrix[m][i][center[m][k]];
                        map[m][i] = k;
                        // printf("map[%d][%d] = %d\n", m, i, k);
                    }
                }
            }

            // 重新选取中心点
            for(i = 0; i < num; i++)
            {
                dist_temp = 0; // 记录到同一个簇其余（控制器）点的最短距离之和
                for(j = 0; j < num; j++)
                {
                    if(map[m][i] == map[m][j] && ctrl_flag[j] == 1)
                    {
                        dist_temp += matrix[m][i][j];
                    }
                }
                if(dist_temp < mindist[m][map[m][i]])
                {
                    mindist[m][map[m][i]] = dist_temp;
                    center_temp[m][map[m][i]] = i;
                    // printf("center_temp[%d][%d] = %d\n", m, map[m][i], i);
                }
            }

            dist_temp = 0; // mindist求和，评估聚类效果
            for(k = 1; k <= center_num; k++)
            {
                center[m][k] = center_temp[m][k];
                dist_temp += mindist[m][k];
                // printf("center[%d][%d] = %d\n", m, k, center[m][k]);
            }
            optval = dist_sum - dist_temp;
            dist_sum = dist_temp;
            // printf("test_%d optval = %d\n", m, optval);
            // printf("test_%d dist_sum = %d\n", m, dist_sum);
        }
        // printf("\n");
        for(k = 1; k <= center_num; k++)
        {
            alter_location[center[m][k]] = 1; // 标记备选节点
        }
    }

    // 利用贪婪取走启发式算法(Greedy Dropping Heuristic Algorithm)，删减备选位置
    alter_num = 0; // 记录备选中心节点个数
    // printf("备选中心节点：");
    for(i = 0; i < num; i++)
    {
        if(alter_location[i] == 1)
        {
            alter_num++;
            alter_node[alter_num] = i; // 记录备选中心节点
            // printf("%d ", i);
        }
    }
    // printf("\n");

    // 聚类将每个节点分配给最近的备选中心节点
    dist_sum = 0; // 记录所有时间片的总距离
    for(m = 0; m < slot_num; m++)
    {
        for(i = 0; i < num; i++)
        {
            dist_temp = maxdist; // 记录每个点到各个中心点的最小距离
            for(k = 1; k <= alter_num; k++)
            {
                if(matrix[m][i][alter_node[k]] < dist_temp)
                {
                    dist_temp = matrix[m][i][alter_node[k]];
                    map[m][i] = k;
                    // printf("map[%d][%d] = %d\n", m, i, k);
                }
            }
            if(ctrl_flag[i] == 1)
                dist_sum += dist_temp;
        }
    }

    // 循环删除备选中心节点，直到中心节点个数为K
    // printf("alter_num=%d, K=%d\n", alter_num, K);
    for(n = alter_num; n > K; n--) 
    {
        memset(cost, 0, sizeof(cost));
        memset(funval, 0, sizeof(funval));
        for(k = 1; k <= alter_num; k++)// 遍历备选中心节点，计算删除之后的目标函数值
        {
            for(m = 0; m < slot_num; m++)
            {
                for(i = 0; i < num; i++)
                {
                    if(map[m][i] == k && ctrl_flag[i] == 1)
                    {
                        cost[m][i][k] -= matrix[m][i][alter_node[k]];
                        // printf("matrix[%d][%d][alter_node[%d](%d)] = %d\n", m, i, k, alter_node[k], matrix[m][i][alter_node[k]]);
                        dist_temp = maxdist; // 记录到各个中心点的最小距离
                        for(j = 1; j <= alter_num; j++)
                        {
                            if(matrix[m][i][alter_node[j]] < dist_temp && j != k && flag[j] != 1)
                            {
                                dist_temp = matrix[m][i][alter_node[j]];
                                // map[m][i] = k;
                                // printf("map[%d][%d] = %d\n", m, i, k);
                                // printf("center = alter_node[%d] = %d\n", j, alter_node[j]);
                            }
                        }
                        cost[m][i][k] += dist_temp;
                        // printf("dist_temp = %d\n", dist_temp);
                        // printf("cost[%d][%d][%d] = %d\n", m, i, k, cost[m][i][k]);
                        funval[k] += cost[m][i][k];
                        // printf("funval[%d] = %d\n", k, funval[k]);
                        // printf("\n");
                    }
                }
            }
        }
        // 选取代价最小的节点删除
        minval = maxdist;
        for(k = 1; k <= alter_num; k++)
        {
            if(funval[k] < minval && flag[k] != 1)
            {
                minval = funval[k];
                minnode = k;
            }
        }
        dist_sum += minval;
        flag[minnode] = 1;
        // printf("del %d\n", alter_node[minnode]);
        for(m = 0; m < slot_num; m++)
        {
            for(i = 0; i < num; i++)
            {
                if(map[m][i] == minnode && ctrl_flag[i] == 1)
                {
                    dist_temp = maxdist; // 记录到各个中心点的最小距离
                    k = 0;
                    for(j = 1; j <= alter_num; j++)
                    {
                        if(matrix[m][i][alter_node[j]] < dist_temp && flag[j] != 1)
                        {
                            dist_temp = matrix[m][i][alter_node[j]];
                            k = j;
                        }
                    }
                    map[m][i] = k;
                }
            }
        }
    }

    // printf("K = %d, dist_sum = %d\n", K, dist_sum);
    // printf("中心节点： ");
    // for(k = 1; k <= alter_num; k++)
    // {
    //     if(flag[k] != 1)
    //     {
    //         printf("%d ", alter_node[k]);
    //     }
    // }
    // printf("\n");

    snprintf(fname, fname_len, "db");
    if((fp=fopen(fname, "w"))==NULL)
    {
        printf("打开文件%s错误\n", fname);
        return -1;
    }

    fprintf(fp, "%d %d\n", K, dist_sum);
    for(k = 1; k <= alter_num; k++)
    {
        if(flag[k] != 1)
        {
            fprintf(fp, "%d ", alter_node[k]);
        }
    }
    fprintf(fp, "\n");
    fclose(fp);

    for(fnum = 0; fnum < slot_num; fnum++)
    {
        snprintf(fname, fname_len, "db_%d", fnum);
        if((fp=fopen(fname, "w"))==NULL)
        {
            printf("打开文件%s错误\n", fname);
            return -1;
        }

        for(k = 1; k <= alter_num; k++)
        {
            if(flag[k] != 1)
            {
                fprintf(fp, "%d\n ", alter_node[k]);
                for(i = 0; i < num; i++)
                {
                    if(map[fnum][i] == k && ctrl_flag[i] == 1)
                    {
                        fprintf(fp, "%d ", i);
                    }
                }
                fseek(fp, -1, SEEK_CUR);
                fprintf(fp, "\n");
            }
        }
    }

    return 0;
}

int main()
{
    int K = 1;
    // for(K = 1; K <= maxnum; K++)
    // {
    //     printf("%d ", cv(K));
    // }
    // printf("\n");

    K = 6; // 数据库数量（手动选取）
    cal(K);

    return 0;
}

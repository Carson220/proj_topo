// 考虑单个时间片，按距离聚类部署SDN控制器

#include <stdio.h>
#include <string.h>

// #define K 7 // controller num
#define slot_num 44
// #define fname "test_0"
#define fname_len 20
#define maxnum 66
#define maxdist 0x3f3f3f3f

// 用于交叉验证选取合适的K值
int cv(int K, int fnum)
{
   FILE *fp = NULL;
   char fname[fname_len] = {0,};
   int node1, node2, num = 0;
   float dist = 0;
   int dist_temp = 0;
   int mindist[K+1]; // 记录每个簇的最短距离和的最小值
   int dist_sum = 2e9; // mindist求和，评估聚类效果
   int optval = 2e9; // 记录每次聚类的优化值

   int map[maxnum]; // 记录每个点所属的簇 map[0~65] = 1~k
   int center[K+1]; // 记录每个簇的中心点 center[1~k] = 0~65
   int center_temp[K+1]; // 记录每个簇的临时中心点
   int matrix[maxnum][maxnum];

   int i, j, k = 0;
   memset(mindist, 0x3f, sizeof(mindist));
   memset(map, 0, sizeof(map));
   memset(center, 0, sizeof(center));
   memset(center_temp, 0, sizeof(center_temp));
   memset(matrix, 0x3f, sizeof(matrix));

   snprintf(fname, fname_len, "test_%d", fnum);
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
               matrix[i][j] = matrix[i][k] + matrix[k][j];
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

   // K-means 聚类
   // 随机选取中心点
   for(k = 2; k <= K; k++)
   {
      center[k] = center[k-1] + (num/K);
      center_temp[k] = center_temp[k-1] + (num/K);
      // printf("center[%d] = %d\n", k, center[k]);
   }
   while(optval >1e3)
   {
      // 聚类
      for(i = 0; i < num; i++)
      {
         dist_temp = maxdist; // 记录到各个中心点的最小距离
         for(k = 1; k <= K; k++)
         {
            if(matrix[i][center[k]] < dist_temp)
            {
               dist_temp = matrix[i][center[k]];
               map[i] = k;
               // printf("map[%d] = %d\n", i, k);
            }
         }
      }
      // 重新选取中心点
      for(i = 0; i < num; i++)
      {
         dist_temp = 0; // 记录到同一个簇其余点的最短距离之和
         for(j = 0; j < num; j++)
         {
            if(map[i] == map[j] && i != j)
            {
               dist_temp += matrix[i][j];
            }
         }
         if(dist_temp < mindist[map[i]])
         {
            mindist[map[i]] = dist_temp;
            center_temp[map[i]] = i;
            // printf("center_temp[%d] = %d\n", map[i], i);
         }
      }
      dist_temp = 0; // mindist求和，评估聚类效果
      for(k = 1; k <= K; k++)
      {
         center[k] = center_temp[k];
         dist_temp += mindist[k];
         // printf("center[%d] = %d\n", k, center[k]);
      }
      optval = dist_sum - dist_temp;
      dist_sum = dist_temp;
      // printf("optval = %d\n", optval);
      // printf("dist_sum = %d\n", dist_temp);
   }

   return dist_temp;
}

// 用于根据给定的K值计算控制器部署方案，并写入文件
int cal(int K, int fnum)
{
   FILE *fp = NULL;
   char fname[fname_len] = {0,};
   int node1, node2, num = 0;
   float dist = 0;
   int dist_temp = 0;
   int mindist[K+1]; // 记录每个簇的最短距离和的最小值
   int dist_sum = 2e9; // mindist求和，评估聚类效果
   int optval = 2e9; // 记录每次聚类的优化值

   int map[maxnum]; // 记录每个点所属的簇 map[0~65] = 1~k
   int center[K+1]; // 记录每个簇的中心点 center[1~k] = 0~65
   int center_temp[K+1]; // 记录每个簇的临时中心点
   int matrix[maxnum][maxnum];

   int standby_ctrl = -1; // 记录备份控制器的所在节点编号
   int standby_dist = 2e9; // 记录备份控制器到交换机的最小距离

   int i, j, k = 0;
   memset(mindist, 0x3f, sizeof(mindist));
   memset(map, 0, sizeof(map));
   memset(center, 0, sizeof(center));
   memset(center_temp, 0, sizeof(center_temp));
   memset(matrix, 0x3f, sizeof(matrix));

   snprintf(fname, fname_len, "test_%d", fnum);
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
               matrix[i][j] = matrix[i][k] + matrix[k][j];
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

   // K-means 聚类
   // 随机选取中心点
   for(k = 2; k <= K; k++)
   {
      center[k] = center[k-1] + (num/K);
      center_temp[k] = center_temp[k-1] + (num/K);
      // printf("center[%d] = %d\n", k, center[k]);
   }
   while(optval >1e3)
   {
      // 聚类
      for(i = 0; i < num; i++)
      {
         dist_temp = maxdist; // 记录到各个中心点的最小距离
         for(k = 1; k <= K; k++)
         {
            if(matrix[i][center[k]] < dist_temp)
            {
               dist_temp = matrix[i][center[k]];
               map[i] = k;
               // printf("map[%d] = %d\n", i, k);
            }
         }
      }
      // 重新选取中心点
      for(i = 0; i < num; i++)
      {
         dist_temp = 0; // 记录到同一个簇其余点的最短距离之和
         for(j = 0; j < num; j++)
         {
            if(map[i] == map[j] && i != j)
            {
               dist_temp += matrix[i][j];
            }
         }
         if(dist_temp < mindist[map[i]])
         {
            mindist[map[i]] = dist_temp;
            center_temp[map[i]] = i;
            // printf("center_temp[%d] = %d\n", map[i], i);
         }
      }
      dist_temp = 0; // mindist求和，评估聚类效果
      for(k = 1; k <= K; k++)
      {
         center[k] = center_temp[k];
         dist_temp += mindist[k];
         // printf("center[%d] = %d\n", k, center[k]);
      }
      optval = dist_sum - dist_temp;
      dist_sum = dist_temp;
      // printf("optval = %d\n", optval);
      // printf("dist_sum = %d\n", dist_temp);
   }

   // printf("test_%d K=%d\n", fnum, K);
   // for(k = 1; k <= K; k++)
   // {
   //    printf("center[%d] = %d ", k, center[k]);
   //    for(i = 0; i < num; i++)
   //    {
   //       if(map[i] == k)
   //          printf("%d ", i);
   //    }
   //    printf("\n");
   // }

   snprintf(fname, fname_len, "ctrl_%d", fnum);
   if((fp=fopen(fname,"w"))==NULL)
   {
      printf("打开文件%s错误\n", fname);
      return -1;
   }
   fprintf(fp, "%d\n", K);
   for(k = 1; k < K; k++)
   {
      fprintf(fp, "%d\n", center[k]);
      for(i = 0; i < num; i++)
      {
         if(map[i] == k)
            fprintf(fp, "%d ", i);
      }
      fseek(fp, -1, SEEK_CUR);
      fprintf(fp, "\n");
   }
   fprintf(fp, "%d\n", center[k]);
   for(i = 0; i < num; i++)
   {
      if(map[i] == k)
         fprintf(fp, "%d ", i);
   }
   fclose(fp);

   // 写入主控制器
   snprintf(fname, fname_len, "active_ctrl_%d", fnum);
   if((fp=fopen(fname,"w"))==NULL)
   {
      printf("打开文件%s错误\n", fname);
      return -1;
   }
   fprintf(fp, "%d\n", num);
   for(i = 0; i < num; i++)
   {
      fprintf(fp, "%d ", center[map[i]]);
   }
   fclose(fp);

   // 遍历选取备份控制器
   snprintf(fname, fname_len, "standby_ctrl_%d", fnum);
   if((fp=fopen(fname,"w"))==NULL)
   {
      printf("打开文件%s错误\n", fname);
      return -1;
   }
   fprintf(fp, "%d\n", num);
   for(i = 0; i < num; i++)
   {
      standby_dist = 2e9;
      for(k = 1; k <= K; k++)
      {
         if(k != map[i])
         {
            if(matrix[i][center[k]] < standby_dist)
            {
               standby_dist = matrix[i][center[k]];
               standby_ctrl = center[k];
            }
         }
      }
      fprintf(fp, "%d ", standby_ctrl);
   }
   fclose(fp);

   // for(i = 0; i < num; i++)
   // {
   //    standby_dist = 2e9;
   //    for(k = 1; k <= K; k++)
   //    {
   //       if(k != map[i])
   //       {
   //          if(matrix[i][center[k]] < standby_dist)
   //          {
   //             standby_dist = matrix[i][center[k]];
   //             standby_ctrl = center[k];
   //          }
   //       }
   //    }
   //    printf("%d %d\n", i, standby_ctrl);
   // }

   return 0;
}

int main()
{
   int K, fnum = 0;
   for(fnum = 0; fnum < slot_num; fnum++)
   {
      // printf("test_%d ", fnum);
      for(K = 1; K <= maxnum; K++)
      {
         // printf("%d ", cv(K, fnum));
      }
      // printf("\n");
   }

   int ctrl_num[slot_num] = { 7, 7, 7, 7, 7, 5, 5, 5, 5, 7, 7, \
                              5, 7, 7, 5, 5, 5, 7, 7, 5, 5, 5, \
                              7, 7, 7, 7, 7, 5, 5, 5, 5, 7, 7, \
                              7, 7, 7, 7, 5, 5, 7, 7, 5, 5, 5  \
   }; // 记录每个时间片的控制器数量（手动选取）

   for(fnum = 0; fnum < slot_num; fnum++)
   {
      cal(ctrl_num[fnum], fnum);
   }

   return 0;
}

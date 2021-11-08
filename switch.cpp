/***************************************************************
*   文件名称：switch.cpp
*   描    述：用于转换拓扑，计算时间片切换的默认路由 
***************************************************************/
#include<bits/stdc++.h> 
#include <set>
using namespace std;

#define slot_num 44
#define fname_len 50

int main()
{
    FILE *fp1, *fp2 = NULL;
    char fname[fname_len] = {0,};
    int node1, node2, num = 0;
    float dist = 0;
    set<pair<int, int>> pre_topo;
    int i, j = 0;

    for(i = 0; i < slot_num; i++)
    {
        j = (i + slot_num - 1) % slot_num; // pre topo index
        pre_topo.clear();

        snprintf(fname, fname_len, "./test/test_%d", j);
        if((fp1 = fopen(fname,"r"))==NULL)
        {
            printf("打开文件%s错误\n", fname);
            return -1;
        }
        fscanf(fp1, "%d", &num);

        while(!feof(fp1))
        {
            fscanf(fp1, "%d %d %f\n", &node1, &node2, &dist);
            //printf("%d %d %f\n", node1, node2, dist);
            pre_topo.insert(pair<int, int>(node1, node2));
        }
        fclose(fp1);

        snprintf(fname, fname_len, "./test/test_%d", i);
        if((fp1 = fopen(fname,"r"))==NULL)
        {
            printf("打开文件%s错误\n", fname);
            return -1;
        }
        fscanf(fp1, "%d", &num);
        snprintf(fname, fname_len, "./switch_topo/switch_%d", i);
        if((fp2 = fopen(fname,"w"))==NULL)
        {
            printf("打开文件%s错误\n", fname);
            return -1;
        }
        fprintf(fp2, "%d\n\n", num);
        while(!feof(fp1))
        {
            fscanf(fp1, "%d %d %f\n", &node1, &node2, &dist);
            //printf("%d %d %f\n", node1, node2, dist);
            if(pre_topo.count(pair<int, int>(node1, node2)) != 0)
            {
                fprintf(fp2, "%d %d %f\n", node1, node2, dist);
            }
            // else
            // {
            //     printf("%d %d %f\n", node1, node2, dist);
            // }
        }
        // printf("\n");
        fclose(fp1);
        fclose(fp2);
    }
    
    return 0;
}

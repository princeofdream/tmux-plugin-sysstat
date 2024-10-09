/*
 * =====================================================================================
 *
 *       Filename:  sysstat.cpp
 *
 *    Description:  Description
 *
 *        Version:  1.0
 *        Created:  2024年10月09日 11时23分59秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  lijin (jin), jinli@syncore.space
 *   Organization:  SYNCORE
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

enum DispColorSt {
    DISP_COLOR_LIGHT_GREEN = 0x5fff00,
    DISP_COLOR_GREEN       = 0x98c379,
    DISP_COLOR_YELLOW      = 0xe5c07b,
    DISP_COLOR_RED         = 0xe06c75,
};

// 用来存储内存信息的结构体
typedef struct {
    long long totalMem;
    long long freeMem;
} MemoryInfo;

// 从 /proc/meminfo 文件中读取内存信息
MemoryInfo getMemoryInfo() {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) {
        perror("Failed to open /proc/meminfo");
        exit(EXIT_FAILURE);
    }

    MemoryInfo memInfo = {0, 0};
    char line[256];

    while (fgets(line, sizeof(line), fp)) {
        long long value;
        if (sscanf(line, "MemTotal: %lld kB", &value) == 1) {
            memInfo.totalMem = value;
        }
        if (sscanf(line, "MemFree: %lld kB", &value) == 1) {
            memInfo.freeMem = value;
        }
        // 如果还需要其他内存信息，可以在这里继续添加解析代码
    }

    fclose(fp);
    return memInfo;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// 用来存储CPU时间片的结构体
typedef struct {
    long long user;
    long long nice;
    long long system;
    long long idle;
    long long iowait;
    long long irq;
    long long softirq;
    long long steal;
    long long guest;
    long long guest_nice;
} CpuTime;

// 从 /proc/stat 文件中读取CPU时间片
int readCpuTime(CpuTime *result) {
    FILE *fp;
    char buffer[1024];
    char cpu[5];
    fp = fopen("/proc/stat", "r");
    if (fp == NULL) {
        perror("Failed to open /proc/stat");
        return -1;
    }
    if (fgets(buffer, sizeof(buffer), fp) == NULL) {
        perror("Failed to read /proc/stat");
        fclose(fp);
        return -1;
    }
    fclose(fp);
    sscanf(buffer, "%s %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld",
           cpu,
           &result->user,
           &result->nice,
           &result->system,
           &result->idle,
           &result->iowait,
           &result->irq,
           &result->softirq,
           &result->steal,
           &result->guest,
           &result->guest_nice);
    return 0;
}

double calculateCpuUsage(const CpuTime *start, const CpuTime *end) {
    long long idle_diff = end->idle - start->idle;
    long long total_diff = (end->user + end->nice + end->system + end->idle + end->iowait + end->irq + end->softirq + end->steal)
                         - (start->user + start->nice + start->system + start->idle + start->iowait + start->irq + start->softirq + start->steal);
    if (total_diff == 0) return 0.0;
    return (1.0 - ((double)idle_diff / total_diff)) * 100.0;
}

int cpuUsed() {
    double usePercent;
    uint32_t disColor;

    CpuTime start, end;

    // 读取第一次CPU时间
    if (readCpuTime(&start) != 0) {
        return EXIT_FAILURE;
    }

    // 等待一秒
    sleep(1);

    // 读取第二次CPU时间
    if (readCpuTime(&end) != 0) {
        return EXIT_FAILURE;
    }

    usePercent = calculateCpuUsage(&start, &end);
    if (usePercent < 75) {
        disColor = DISP_COLOR_GREEN;
    } else if (usePercent < 90) {
        disColor = DISP_COLOR_YELLOW;
    } else {
        disColor = DISP_COLOR_RED;
    }
    // 计算并打印CPU使用率
    // printf("CPU Usage: %.2f%%", calculateCpuUsage(&start, &end));
    printf("#[bg=#3e4452]U:#[fg=#%x,bg=#3e4452]%.2f\%#[fg=#aab2bf,bg=#3e4452]",
           disColor,usePercent);

    return EXIT_SUCCESS;
}

int memUsed() {
    int usePercent;
    float memDisp;
    uint32_t disColor;

    MemoryInfo memInfo = getMemoryInfo();

    // printf("Total Memory: %lld kB\n", memInfo.totalMem);
    // printf("Free Memory: %lld kB\n", memInfo.freeMem);
    // // 计算已使用的内存大小
    // printf("Used Memory: %lld kB\n", memInfo.totalMem - memInfo.freeMem);
    disColor = 0x98c379;
    usePercent = 10000 - 10000*memInfo.freeMem/memInfo.totalMem;
    if (usePercent < 7500) {
        disColor = DISP_COLOR_GREEN;
    } else if (usePercent < 9000) {
        disColor = DISP_COLOR_YELLOW;
    } else {
        disColor = DISP_COLOR_RED;
    }

    memDisp = 12.3;
    memDisp = usePercent/100.0;
    printf(" #[bg=#3e4452]M:#[fg=#%x,bg=#3e4452]%.2f\%#[fg=#aab2bf,bg=#3e4452]  #[default]",
          disColor,memDisp);
    return 0;
}

int loadavgStat()
{
    double loadavg[3];
    uint32_t disColor[3];
    int itemCnt;

    long nprocs = -1;
    // long nprocs_max = -1;

    nprocs = sysconf(_SC_NPROCESSORS_ONLN);
    // nprocs_max = sysconf(_SC_NPROCESSORS_CONF);

    if (nprocs < 1) {
        fprintf(stderr, "Could not determine number of CPUs online:\n");
        // return 1;
    // } else if (nprocs_max < 1) {
    //     fprintf(stderr, "Could not determine number of CPUs configured:\n");
    //     // return 1;
    }

    // printf("Number of CPUs online: %ld\n", nprocs);
    // printf("Number of CPUs configured: %ld\n", nprocs_max);

    // 读取负载均衡值
    if (getloadavg(loadavg, 3) == -1) {
        perror("loadavgNG");
        return 1;
    }

    itemCnt = 0;
    while (itemCnt < sizeof(loadavg)/sizeof(loadavg[0])) {
        loadavg[itemCnt] = loadavg[itemCnt]/nprocs;
        if (loadavg[itemCnt] < 0.75) {
            disColor[itemCnt] = DISP_COLOR_GREEN;
        } else if (loadavg[itemCnt] < 0.9) {
            disColor[itemCnt] = DISP_COLOR_YELLOW;
        } else {
            disColor[itemCnt] = DISP_COLOR_RED;
        }
        itemCnt++;
    }
    // 打印1分钟、5分钟和15分钟的负载均衡值
    // printf("1 minute load average: %.2f\n", loadavg[0]);
    // printf("5 minute load average: %.2f\n", loadavg[1]);
    // printf("15 minute load average: %.2f\n", loadavg[2]);
    printf("#[fg=#%x,bg=#3e4452]%.2f \
#[default]#[fg=#%x,bg=#3e4452]%.2f \
#[default]#[fg=#%x,bg=#3e4452]%.2f \
#[default]#[bg=#3e4452]",
    disColor[0],loadavg[0],
    disColor[1],loadavg[1],
    disColor[2],loadavg[2]);
    return 0;
}

int main(int argc, char *argv[])
{
    cpuUsed();
    memUsed();
    loadavgStat();
    return 0;
}





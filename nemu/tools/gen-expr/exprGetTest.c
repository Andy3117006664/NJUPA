#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
 
int main(int argc, char *argv[]) {
	char code_buf[]=
	"#include <stdio.h>\n"
	"int main() { "
	"  unsigned a = 2;"
	"  unsigned b = 0;"
	"  unsigned ans = a / b;"
	"  printf(\"%u\", ans); "
	"  return 0; "
	"}";
	
	FILE *fp = fopen("/tmp/.code.c", "w");
	assert(fp != NULL);
	fputs(code_buf, fp);
	fclose(fp);
 
	int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    //if (ret != 0) continue; 避免报除零错误，把全部的warning看做error处理
	if (ret != 0) {
   		printf("call system error\n");
    	return 0;
	}
 
	fp = popen("/tmp/.expr", "r");
	assert(fp != NULL);
 
	int result = -1;
	ret = fscanf(fp, "%d", &result);
	int status = 0;
	status = pclose(fp);
	printf("status: %d", status);
	// 检查子进程的终止状态
	if (WIFEXITED(status)) {
    	printf("子进程正常终止，退出状态码：%d\n", WEXITSTATUS(status));
	} else if (WIFSIGNALED(status)) {
    	printf("子进程被信号终止，信号编号：%d\n", WTERMSIG(status));
	}
	printf("%u\n", result);
	return 0;
}
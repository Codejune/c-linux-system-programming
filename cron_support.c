/**
 * @file cron_support.c
 * @brief ssu_crontab, ssu_crond에서 공통적으로 사용되는 함수 
 * @author 김병준 (kbj9704@gmail.com)
 */

#include "cron_support.h"

extern char reservation_command[BUFFER_SIZE][MAX_BUFFER_SIZE];

/**
 * @brief 입력한 명령행을 토큰 구조체로 변환
 * @param command 명령행 토큰 구조체
 * @param command_buffer 명령 문자열
 */
void make_command_token(CommandToken *command, char *command_buffer) // 입력한 명령행을 토큰 구조체로 변환
{
	char *tmp;

	command->argv = (char**)calloc(BUFFER_SIZE, sizeof(char*));
	command->argc = 0;

	if ((tmp = strtok(command_buffer, " ")) == NULL)
		return;
#ifdef DEBUG
	printf("make_command_token(): command->argv[%d] = %s\n", command->argc, tmp);
#endif

	command->argv[command->argc] = (char *)calloc(BUFFER_SIZE, sizeof(char));
	strcpy(command->argv[command->argc++], tmp);

	while ((tmp = strtok(NULL, " ")) != NULL) {
#ifdef DEBUG
		printf("make_command_token(): command->argv[%d] = %s\n", command->argc, tmp);
#endif
		command->argv[command->argc] = (char *)calloc(BUFFER_SIZE, sizeof(char)); // 명령행 인자 메모리 공간 할당
		strcpy(command->argv[command->argc++], tmp);
	}
}

/**
 * @brief 명령행 구조체 초기화
 * @param command 명령행 구조체
 */
void free_command_token(CommandToken *command) // 명령행 구조체 초기화
{
	int i;

	for (i = 0; i < command->argc; i++)
		free(command->argv[i]);
	free(command->argv);
}

/**
 * @brief 예약 명령 목록 가져오기
 */
int get_reservation_command(void) // 예약 명령 목록 가져오기
{
	FILE *fp;

	int count = 0;

	if ((fp = fopen(CRONTAB_FILE, "r+")) == NULL) {
		fprintf(stderr, "get_reservation_command: fopen error for %s\n", CRONTAB_FILE);
		return count;
	}

	while(fscanf(fp, "%[^\n]\n", reservation_command[count]) > 0)
		count++;
#ifdef DEBUG
	printf("get_reservation_command: reservation_count = %d\n", count);
#endif

	fclose(fp);
	return count;
}

/**
 * @brief 로그 파일에 이력 기록
 * @param command_type 명령 타입 번호
 * @param command 명령 문자열
 */
void write_log(int command_type, char *command) // 로그 파일에 이력 기록
{
	FILE *fp;
	time_t now_t;
	struct tm *now_tm;

	if ((fp = fopen(CRONTAB_LOG, "r+")) == NULL) {
		fprintf(stderr, "write_log: fopen error for %s\n", CRONTAB_LOG);
		return;
	}
	fseek(fp, 0, SEEK_END);

	time(&now_t);
	now_tm = localtime(&now_t);

	switch (command_type) {
		case ADD:
			fprintf(fp, "[%.24s] %s %s\n", asctime(now_tm), "add", command);
			break;
		case REMOVE:
			fprintf(fp, "[%.24s] %s %s\n", asctime(now_tm), "remove", command);
			break;
		case RUN:
			fprintf(fp, "[%.24s] %s %s\n", asctime(now_tm), "run", command);
			break;
	}
	fclose(fp);
}



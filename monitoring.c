#include "monitoring.h"

char pwd[BUFFER_SIZE];
char check_path[BUFFER_SIZE]; // $(PWD)/check 절대경로
char log_path[BUFFER_SIZE];
change_file change_list[MAX_BUFFER_SIZE]; // 변경 목록

int main(void) // 모니터링 메인 함수
{
	FILE *fp; // log.txt 파일 구조체

	// 프로세스 이름 변경
	//prctl(PR_SET_NAME, "ssu_mntr-daemon", NULL, NULL, NULL);

	getcwd(pwd, BUFFER_SIZE);
	sprintf(check_path, "%s/%s", pwd, CHECK);
	sprintf(log_path, "%s/%s", pwd, LOG);

	if(access(CHECK, F_OK) < 0) // 모니터링 디렉토리 확인
		mkdir(CHECK, DIR_MODE); // 존재하지 않을 경우 생성

	if((fp = fopen(LOG, "w+")) == NULL) { // 로그 파일 열기 및 생성, 기존 로그 덮어쓰기
		fprintf(stderr, "fopen error for log.txt\n");
		exit(1);
	}
	fclose(fp);


	set_daemon_process();
	monitoring();

	exit(0);
}

void monitoring(void) // 모니터링
{
	int old_list_cnt, new_list_cnt; // 모니터링 디렉토리 파일 개수(기존, 신규)
	file_node *old_list, *new_list; // 모니터링 디렉토리 트리(기존, 신규)
	int is_first;
	int change_list_cnt;

	is_first = true;

	while(true) {
		new_list = make_list(check_path); // 현재 파일 목록 및 상태 저장
		new_list_cnt = count_file(new_list); // 현재 목록에 존재하는 파일 개수
		init_list_status(new_list->child, UNCHCK); // 현재 파일 목록 모니터링 상태 초기화

		if(is_first) { // 최초 실행일 경우
			old_list = new_list;
			old_list_cnt = new_list_cnt;
			is_first = false;
			continue;
		}
		
		compare_list(new_list->child, old_list->child);
		change_list_cnt = write_change_list(new_list->child, 0, CREATE);
		change_list_cnt = write_change_list(old_list->child, change_list_cnt, DELETE);
		sort_change_list(change_list_cnt);

		write_change_log(change_list_cnt);
		old_list = new_list;
		old_list_cnt = new_list_cnt;
		init_list_status(old_list->child, UNCHCK);
	}
}


void init_list_status(file_node *head, int status) // 모니터링 파일 상태 초기화
{
	file_node *now;

	now = head;

	while(true) {
		now->status = status;

		if(S_ISDIR(now->attr.st_mode))  // 디렉토리의 경우
			if(now->child != NULL)  
				init_list_status(now->child, status); // 디렉토리 하위 파일 모니터링 상태 초기화

		if(now->next != NULL)
			now = now->next;

		else break;
	}
}

int count_file(file_node *head) // 파일 개수 반환 
{
	int cnt;
	file_node *now;

	cnt = 0;
	now = head->child; // 주어진 루트노드의 자식으로 시작

	while(true) { // 개수 탐색 시작
		if(now->child != NULL) { // 현재 탐색하는 파일이 디렉토리일 경우

			cnt += count_file(now); // 해당 디렉토리 파일 개수 재귀 탐색
			now = now->next; // 다음 파일 탐색

			if(now == NULL) // 다음 파일이 더이상 없을 경우
				break;
		} else if(now->next != NULL) // 다음 탐색할 파일이 존재할 경우(현재 디렉토리의 파일 목록 끝)
			now = now->next;  // 다음 파일 탐색
		else break;
	}
	return cnt;
}

void compare_list(file_node *new_list, file_node *old_list) // 파일 목록 트리 비교
{
	file_node *now;

	now = old_list;

	while(true) {	
		compare_file(new_list, now);

		if(S_ISDIR(now->attr.st_mode)) 
			if(now->child != NULL)
				compare_list(new_list, now->child);

		if(now->next != NULL)
			now = now->next;

		else break;
	}
}

int compare_file(file_node *new_file, file_node *old_file) // 파일 정보 비교
{
	file_node *now;

	now = new_file;

	while(true) {
		if(!strcmp(now->name, old_file->name)) { // 해당 이름을 가진 파일이 기존에 이미 존재할 경우
			now->status = CHCKED;

			if(now->attr.st_mtime != old_file->attr.st_mtime) // 해당 파일이 수정되었을 경우
				now->status = MODIFY;

			old_file->status = CHCKED;
			return true;
		}

		if(S_ISDIR(now->attr.st_mode)) // 현재 파일이 디렉토리인 경우
			if(now->child != NULL) // 디렉토리 안에 파일이 존재할 경우
				if(compare_file(now->child, old_file)) 
					break;

		if(now->next != NULL)
			now = now->next;
		else
			break;
	}
	return false;
}

int write_change_list(file_node *head, int idx, int status) // 변경사항 목록 작성
{
	file_node *now;

	now = head;

	while(true) {
		switch(now->status) {
			case UNCHCK:
				if(status == CREATE)
					change_list[idx].time = now->attr.st_mtime;
				else if(status == DELETE)
					change_list[idx].time = time(NULL);
				strcpy(change_list[idx].name, now->name);
				change_list[idx++].status = status;
				break;
			case MODIFY:
				change_list[idx].time = now->attr.st_mtime;
				strcpy(change_list[idx].name, now->name);
				change_list[idx++].status = MODIFY;
				break;
		}

		if(S_ISDIR(now->attr.st_mode))
			if(now->child != NULL)
				idx = write_change_list(now->child, idx, status);

		if(now->next != NULL)
			now = now->next;

		else break;
	}

	return idx;
}

void sort_change_list(int idx) // 변경사항 목록 시간순 정렬
{
	int i, j;
	change_file tmp;

	for(i = 0; i < idx; i++)
		for(j = i+1; j < idx; j++)
			if(change_list[i].time > change_list[j].time) {
				tmp = change_list[i];
				change_list[i] = change_list[j];
				change_list[j] = tmp;
			}
}


void write_change_log(int idx) // 변경사항 파일 기록
{
	char file_name[BUFFER_SIZE];
	char *time_format;
	struct tm time;
	FILE *fp;
	int i;
	char *tmp;
	
	if((fp = fopen(log_path, "r+")) < 0) {
		fprintf(stderr, "fopen error for %s\n", LOG);
		exit(1);
	}
	fseek(fp, 0, SEEK_END); // 오프셋을 로그파일의 끝으로 지정

	for(i = 0; i < idx; i++) {

		tmp = strstr(change_list[i].name, CHECK);
		tmp += strlen(CHECK) + 1; // strlen("check") + 1(/) 
		strcpy(file_name, tmp);

		while((tmp = strchr(file_name, '/')) != NULL) // file_name = "파일명_"
			*tmp = '_';

		time = *localtime(&change_list[i].time);
		time_format = make_time_format(time);

		switch(change_list[i].status) {
			case CREATE:
				fprintf(fp, "[%s][%s_%s]\n", time_format, "create", file_name);
				break;
			case DELETE:
				fprintf(fp, "[%s][%s_%s]\n", time_format, "delete", file_name);
				break;
			case MODIFY:
				fprintf(fp, "[%s][%s_%s]\n", time_format, "modify", file_name);
				break;
		}
	}
	fclose(fp);
}

void set_daemon_process(void) // 데몬 프로세스 설정
{
	pid_t pid;
	int fd, maxfd;

	if((pid = fork()) < 0) {
		fprintf(stderr, "fork error\n");
		exit(1);
	} else if(pid != 0)  // #1 백그라운드 수행
		exit(0);

	// #2 새로운 프로세스 그룹 생성
	setsid(); 

	// #3 터미널 입출력 시그널 무시
	signal(SIGTTIN, SIG_IGN); // STDIN 무시
	signal(SIGTTOU, SIG_IGN); // STDOUT 무시
	signal(SIGTSTP, SIG_IGN); // STDERR 무시

	// #4 파일 모드 생성 마스크 해제
	umask(false);

	// #5 루트 디렉토리 이동
	chdir("/");

	
	// #6 모든 파일 디스크럽터 연결 종료
	maxfd = getdtablesize(); // 모든 파일 디스크럽터 개수 획득
	for(fd = 0; fd < maxfd; fd++) 
		close(fd);

	// #7 표준 입출력 및 에러 재지정
	fd = open("dev/null", O_RDWR); // STDIO 재설정
	dup(0);
	dup(0);

	// 라이브러리 루틴 무효화
}

/**
 * @file ssu_rsync.c
 * @brief ssu_rsync 프로그램
 * @author 김병준 (kbj9704@gmail.com)
 */
#include "ssu_rsync.h"

// 경로
char pwd[MAX_BUFFER_SIZE] = { 0 }; // 프로그램 실행 경로
char src_path[MAX_BUFFER_SIZE] = { 0 }; // 타겟 경로
char dst_path[MAX_BUFFER_SIZE] = { 0 }; // 동기화 경로
char swap_path[MAX_BUFFER_SIZE] = { 0 }; // 스왑 파일

bool option_r = false; // R 옵션
bool option_t = false; // T 옵션
bool option_m = false; // M 옵션
bool is_complete = false; // 동기화 완료 확인

int in_fd; // 표준 입력
int out_fd; // 표준 출력
int err_fd; // 표준 에러

change_file change_list[BUFFER_SIZE]; // 변경 목록

/**
 * @brief ssu_rsync 메인 함수
 * @param argc 인자 개수
 * @param argv 인자 문자열
 */
int main(int argc, char *argv[])
{
	// 실행 시간 측정 
	struct timeval begin_t, end_t;

	// swap 생성
	char swap_path[MAX_BUFFER_SIZE];
	char command[MAX_BUFFER_SIZE];

	// 유효 검사
	char opt;
	struct stat statbuf;
	bool is_invalid = false;
	bool is_src = false;
	bool is_dst = false;

	gettimeofday(&begin_t, NULL); // 측정 시작

	if (argc < 3) { // 인자 개수가 부족할 경우
		fprintf(stderr, "ssu_rsync(): Usage: %s [OPTION] <SOURCE> <DESTINATION>\n", argv[0]);
		exit(1);
	}

	getcwd(pwd, MAX_BUFFER_SIZE);
	signal(SIGUSR1, swap_handler);
	signal(SIGUSR2, swap_handler);

	for (int i = 1; i < argc; i++) {
#ifdef DEBUG
		printf("ssu_rsync(): argv[%d] = %s\n", i, argv[i]);
#endif
		// 옵션 생략
		if (argv[i][0] == '-') 
			continue;

		// 타겟 경로 파싱
		if (!is_src) {
			if (access(argv[i], F_OK) < 0) {
#ifdef DEBUG
				fprintf(stderr, "ssu_rsync(): access error for %s\n", argv[i]);
#endif
				is_invalid = true;
				break;
			}

			realpath(argv[i], src_path); // 절대 경로로 변환
#ifdef DEBUG
			printf("ssu_rsync(): src_path = %s\n", src_path);
#endif
			is_src = true;
			continue;
		}

		// 동기화 경로 파싱
		if (!is_dst) {
			if (access(argv[i], F_OK) < 0) {
#ifdef DEBUG
				fprintf(stderr, "ssu_rsync(): access error for %s\n", argv[i]);
#endif
				is_invalid = true;
				break;
			}

			realpath(argv[i], dst_path); // 동기화 디렉토리 절대 경로 변환
#ifdef DEBUG
			printf("ssu_rsync(): dst_path = %s\n", dst_path);
#endif
			lstat(dst_path, &statbuf); // 동기화 디렉토리 상태 정보 획득
			if (!S_ISDIR(statbuf.st_mode)) { // 동기화 경로가 디렉토리가 아닐 경우
#ifdef DEBUG
				fprintf(stderr, "ssu_rsync(): dst_path doesn't directory\n");
#endif
				is_invalid = true;
				break;
			}
			is_dst = true;
			continue;
		}
	}

	// 옵션 파싱
	while ((opt = getopt(argc, argv, "rtm")) != -1) {
		switch (opt) {
			case 'r':
#ifdef DEBUG
				printf("ssu_rsync(): R option found\n");
#endif
				option_r = true;
				break;
			case 't':
#ifdef DEBUG
				printf("ssu_rsync(): T option found\n");
#endif
				option_t = true;
				break;
			case 'm':
#ifdef DEBUG
				printf("ssu_rsync(): M option found\n");
#endif
				option_m = true;
				break;
			default:
#ifdef DEBUG
				printf("ssu_rsync(): invalid option found\n");
#endif
				is_invalid = true;
				break;
		}
	}

	// 파싱 중 에러 발견
	if (is_invalid)
		exit(1);
	else if (!is_src) {
		fprintf(stderr, "ssu_rsync(): <SOURCE> doesn't exist\n");
		exit(1);
	} else if (!is_dst) {
		fprintf(stderr, "ssu_rsync(): <DESTINATION> doesn't exist\n");
		exit(1);
	}

	sprintf(swap_path, "%s.swp", get_file_name(dst_path)); // swap 파일 경로 생성
	sprintf(command, "tar -cvf %s %s", swap_path, get_file_name(dst_path)); // 명령어 생성
#ifdef DEBUG
	printf("ssu_rsync(): swap_path = %s\n", swap_path);
	printf("ssu_rsync(): command = %s\n", command);
#endif
	kill(getpid(), SIGUSR1); // 표준 입출력 닫음
	system(command); // 명령어 실행(압축)
	kill(getpid(), SIGUSR2); // 표준 입출력 열기

	signal(SIGINT, recovery); // SIGINT 시그널 처리

	syncronize(src_path, dst_path); // 동기화

	remove(swap_path); // swap 파일 삭제

	gettimeofday(&end_t, NULL); // 측정 종료
	ssu_runtime(&begin_t, &end_t); // 실행 시간 출력
	exit(0);
}

/**
 * @brief 표준 입출력 전환
 * @param signo 시그널 
 */
void swap_handler(int signo) // 표준 입출력 전환
{ 
	switch (signo) {
		case SIGUSR1:
			in_fd = dup(0);
			out_fd = dup(1);
			err_fd = dup(2);
			close(0);
			close(1);
			close(2);
			break;
		case SIGUSR2:
			dup2(in_fd, 0);
			dup2(out_fd, 1);
			dup2(err_fd, 2);
			break;
	}
}

/**
 * @brief 동기화 함수
 * @param src_path 타겟 경로
 * @param dst_path 동기화 경로
 */
void syncronize(char *src_path, char *dst_path) // 동기화 함수
{
	file_node *src_list; // 타겟 경로 파일 목록
	file_node *dst_list; // 동기화 경로 파일 목록
	bool is_directory = false;
	int change_count = 0;

	src_list = make_list(src_path); 
	dst_list = make_list(dst_path);

	if (S_ISDIR(src_list->attr.st_mode)) { // 타겟이 디렉토리일 경우
		compare_list(src_list->child, dst_list->child);
		is_directory = true;
	} else // 타겟이 파일일 경우
		compare_file(src_list, dst_list->child, true);

	if (is_directory)
		change_count = write_change_list(src_list->child, change_count, CREATE); // 생성 혹은 수정된 파일 확인
	else 
		change_count = write_change_list(src_list, change_count, CREATE); // 생성 혹은 수정된 파일 확인

	if (option_m)
		change_count = write_change_list(dst_list, change_count, DELETE); // 삭제 혹은 수정된 파일 확인

	free_list(src_list);
	free_list(dst_list);
	is_complete = true;
}

/**
 * @brief 노드 생성
 * @return 새로운 노드
 */
file_node *make_node(void) // 노드 생성
{
	file_node *tmp = calloc(true, sizeof(file_node));

	memset(tmp->name, 0, BUFFER_SIZE);
	tmp->next = NULL;
	tmp->child = NULL;
	tmp->namelist = NULL;
	tmp->size = 0;
	tmp->status = UNCHCK;

	return tmp;
}

/**
 * @brief 디렉토리 파일 목록 트리화
 * @param path 디렉토리 경로
 * @return 트리 루트 노드
 */
file_node *make_list(char *path) // 디렉토리 파일 목록 트리화
{
	file_node *head, *now;
	// 파일    : 노드 생성(절대경로/이름, 상태정보)
	// 디렉토리: 트리 생성
	int file_count;
	int is_dirattr = true;
	int i;

	// 부모: 현재 경로, 디렉토리 상태정보, 파일 목록 정보
	// 자식: 절대경로 파일이름, 파일 상태정보, 다음 파일 포인터 및 디렉토리 하위 부모노드 포인터 
	head = make_node(); 
	now = head;

	strcpy(head->name, path); // 현재 경로 저장
	stat(head->name, &(head->attr)); // 상태 정보 저장

	file_count = scandir(head->name, &(head->namelist), NULL, alphasort); // 현재 경로의 모든 파일 탐색 및 개수 저장
	for(i = 0; i < file_count; i++) {

		if(!strcmp(head->namelist[i]->d_name, ".") || !strcmp(head->namelist[i]->d_name, "..")) // 현재, 상위 디렉토리 접근 지정자 생략
			continue;

		file_node *new = make_node(); // 새로운 노드 생성

		sprintf(new->name, "%s/%s", head->name, head->namelist[i]->d_name); // 파일 이름 저장
		stat(new->name, &(new->attr));

		if(S_ISDIR(new->attr.st_mode)) // 현재 경로의 파일 목록 중 탐색한 파일이 디렉토리일 경우
			new = make_list(new->name); // 해당 디렉토리 파일 목록 트리화
		else 
			new->size = new->attr.st_size;

		if(is_dirattr) { // 현재 노드가 현재 경로의 부모노드일 경우
			now->child = new;
			now = now->child;
			is_dirattr = false;
		} else { // 아닐 경우 형제로 연결
			now->next = new;
			now = now->next;
		}
	}
	head->size = count_size(head);
	return head;
}

/**
 * @brief 디렉토리 크기 반환
 * @param 디렉토리 노드
 * @return 디렉토리 크기
 */
int count_size(file_node *head) // 디렉토리 크기 반환
{
	file_node *now;
	int size;

	size = false;

	if(S_ISDIR(head->attr.st_mode))
		now = head->child;
	else 
		return head->attr.st_size;

	while(now != NULL) {
		size += now->size;
		now = now->next;
	}

	return size;
}

/**
 * @brief 파일 목록 트리 비교 
 * @param src_list 타겟 파일 목록
 * @param dst_list 동기화 디렉토리 파일 목록 
 */
void compare_list(file_node *src_list, file_node *dst_list) // 파일 목록 트리 비교
{
	file_node *now;

	if (src_list == NULL || dst_list == NULL) { // 둘중 하나라도 비교 대상이 존재하지 않을 경우
#ifdef DEBUG
		printf("compare_list(): there is NULL node exist\n");
#endif
		return;
	}

	now = src_list;

	while (now != NULL) { // 타겟 파일 탐색

		compare_file(now, dst_list, true);

		if (option_r) // R 옵션이 존재하는 경우
			if (now->child != NULL)
				compare_list(now->child, dst_list);

		now = now->next;
	}
}

/**
 * @brief 파일 정보 비교
 * @param src_file 타겟 파일 노드
 * @param dst_file 동기화 디렉토리 파일 노드
 * @param is_first 첫번째 레벨 확인 변수
 * @return 비교 성공 유무
 */
bool compare_file(file_node *src_file, file_node *dst_file, bool is_first) // 파일 정보 비교
{
	file_node *now;

	now = dst_file;

	while (now != NULL) {

#ifdef DEBUG
		printf("compare_file(): src_file->name = %s, dst_file->name = %s\n", src_file->name + strlen(pwd) + 1, now->name + strlen(pwd) + 1);
#endif
		if (!strcmp(src_file->name + strlen(pwd) + 1, now->name + strlen(dst_path) + 1)) { // 해당 이름을 가진 파일이 기존에 이미 존재할 경우

#ifdef DEBUG
			printf("compare_file(): file found\n");
#endif
			src_file->status = CHCKED;

			if (src_file->attr.st_mtime != now->attr.st_mtime) { // 해당 파일이 수정되었을 경우
#ifdef DEBUG
				printf("compare_file(): mtime different\n");
#endif
				src_file->status = MODIFY; // 타겟 파일의 상태 변경
			}

			if (src_file->size != now->size) { // 해당 파일의 크기가 변경되었을 경우

#ifdef DEBUG
				printf("compare_file(): size different\n");
#endif
				src_file->status = MODIFY;
			}

			now->status = CHCKED;
#ifdef DEBUG
			printf("compare_file(): src_file->status = %d, dst_file->status = %d\n", src_file->status, now->status);
#endif
			return true;
		}

		if(option_r || is_first)
			if(now->child != NULL) // 디렉토리 안에 파일이 존재할 경우
				if(compare_file(src_file, now->child, false)) 
					break;

		now = now->next;
	}

	return false;
}

/**
 * @brief 변경사항 목록 작성
 * @param head 트리 루트 노드
 * @param idx 변경사항 목록 시작 인덱스
 * @param status 변경 사항 타입 번호
 */
int write_change_list(file_node *head, int idx, int status) // 변경사항 목록 작성
{
	file_node *now;

	now = head;

	while (now != NULL) {

		switch (now->status) {
			case UNCHCK: 
				if (status == CREATE) { // 생성됨
					strcpy(change_list[idx].name, now->name);
					//strcpy(change_list[idx].name, now->name + strlen(src_path) + 1);
					change_list[idx].status = CREATE;
				} else if (status == DELETE) { // 삭제됨
					strcpy(change_list[idx].name, now->name);
					//strcpy(change_list[idx].name, now->name + strlen(dst_path) + 1);
					change_list[idx].status = DELETE;
				}
				change_list[idx++].size = now->size;
#ifdef DEBUG
				printf("write_change_list(): change_list[%d] = %s(%dbyte), status = %d\n", idx - 1, change_list[idx - 1].name, change_list[idx - 1].size, change_list[idx - 1].status);
#endif
				break;

			case MODIFY: // 수정됨
				strcpy(change_list[idx].name, now->name);
				//strcpy(change_list[idx].name, now->name + strlen(src_path) + 1);
				change_list[idx].status = MODIFY;
				change_list[idx++].size = now->size;
#ifdef DEBUG
				printf("write_change_list(): change_list[%d] = %s(%dbyte), status = %d\n", idx - 1, change_list[idx - 1].name, change_list[idx - 1].size, change_list[idx - 1].status);
#endif
				break;
		}

		if(option_r)
			if (now->child != NULL)
				idx = write_change_list(now->child, idx, status);

		now = now->next;
	}

	return idx;
}

/**
 * @brief 모니터링 파일 목록 메모리 할당 해제
 * @param head 트리의 루트 노드
 */
void free_list(file_node *head) // 모니터링 파일 목록 메모리 할당 해제
{
	// 모든 노드들을 찾아서 메모리 할당을 해제한다.
	if(head->child != NULL) // 자식 탐색
		free_list(head->child);

	if(head->next != NULL) // 형제 탐색
		free_list(head->next);

	free(head->namelist);
	free(head); // 메모리 엑세스 허용
}

/**
 * @brief SIGINT 시그널 처리
 * @param signo 시그널
 */
void recovery(int signo) // SIGINT 시그널 처리
{
	char command[MAX_BUFFER_SIZE];

	if(signo == SIGINT) { // SIGINT 시그널 획득 시
#ifdef DEBUG
		printf("recovery(): SIGINT signal is arrived\n");
#endif
		if(is_complete) // 동기화가 완료되었을 경우
			return;

		sprintf(command, "tar -xvf %s", swap_path); // 복원 명령어 생성(압축 해제)
#ifdef DEBUG
		printf("recovery(): command = %s\n", command);
#endif
		remove_directory(dst_path); // 기존 동기화 디렉토리 삭제
		kill(getpid(), SIGUSR1); // 표준 입출력 닫기
		system(command); // 복원 명령어 실행
		kill(getpid(), SIGUSR2);
		remove(swap_path); // swap 파일 삭제
	}
	exit(1);
}

/**
 * @brief 디렉토리 삭제
 * @param path 삭제할 디렉토리 경로
 */
void remove_directory(const char *path) // 디렉토리 삭제
{
	struct dirent *dirp;
	struct stat statbuf;
	DIR *dp;
	char tmp[MAX_BUFFER_SIZE];

	if ((dp = opendir(path)) == NULL)
		return;

	while ((dirp = readdir(dp)) != NULL) { // path에 존재하는 디렉토리 안에 파일들 전부 삭제
		if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
			continue;

		sprintf(tmp, "%s/%s", path, dirp->d_name); // tmp = 디렉토리 내부 파일

		if (lstat(tmp, &statbuf) == -1) // 파일 상태 정보 추출
			continue;

		if (S_ISDIR(statbuf.st_mode)) // 디렉토리일 경우 재귀적으로 제거
			remove_directory(tmp);
		else 
			unlink(tmp);
	}
	closedir(dp);
	rmdir(path);
}

/**
 * @brief 파일명 반환
 * @param path 경로
 * @return 파일명 시작 메모리 주소
 */
char *get_file_name(char *path) // 파일명 추출
{
	char *tmp = path;
	int length = strlen(path);

	for (int i = 0; i < length; i++)
		if(path[i] == '/')
			tmp = path + i;

	return tmp + 1;
}

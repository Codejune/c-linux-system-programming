#include "ssu_mntr.h"

int option_i;
int option_r;
int option_d;
int option_l;

void ssu_mntr(char *pwd) // 프롬프트 메인 함수
{
	char check_path[BUFFER_SIZE]; // $(PWD)/check 절대경로
	char target_path[BUFFER_SIZE];
	char command_line[MAX_BUFFER_SIZE]; // 입력받은 실행 명령 버퍼
	commands command;
	int command_type = false; // 실행 명령 타입
	int level_check[BUFFER_SIZE];

	sprintf(check_path, "%s/%s", pwd, CHECK);

	while (command_type != EXIT) {

		fputs(PROMPT, stdout); // 프롬프트 라인 출력, 20162448> 
		fgets(command_line, sizeof(command_line), stdin); // 실행 명령 입력
		strcpy(command_line, ltrim(rtrim(command_line))); // 실행 명령 좌우 공백 지우기
		command = make_command_token(command_line); 
		command_type = get_command_type(command.argv[0]); // 명령 타입 구분

		/*
		if(!check_option(command.argc, command.argv)) {
			//free_command(command);
			continue;
		}*/
		
		// COMMANDS
		// DELETE(1)  :
		// SIZE(2)    :
		// RECOVER(3) : 
		// TREE(4)    :
		// EXIT(5)    :
		// HELP(6)    : 
		switch(command_type) {
			case DELETE:
				if(command.argc < 2 || (command.argv[1][0] == '-' && command.argc == 2)) { // FILE_NAME이 주어지지 않은 경우
					fprintf(stderr, "%s: <FILE_NAME> doesn't exist\n", command.argv[0]);
					continue;
				}

				strcpy(target_path, command.argv[1]); // FILE_NAME 추출
				chdir(check_path); // 모니터링 디렉토리 이동

				if(access(target_path, F_OK) < 0) // 파일이 존재하지 않을 경우
					fprintf(stderr, "access error for %s\n", target_path);

				realpath(command.argv[1], target_path);
				printf("target_path: %s\n", target_path);

				break;
			case SIZE:
				break;
			case RECOVER:
				break;
			case TREE:
				memset(level_check, 0, sizeof(level_check));
				file_node *head = make_list(check_path);
				print_list_tree(head, 0, level_check, true);
				free_list(head);
				break;
			case EXIT:
				break;
			case HELP:
			case UNKNOWN:
				print_usage();
				break;
			default:
				continue;
		}
		//free_command(command);
		//memset(command_line, 0, sizeof(char));

	}
	fprintf(stdout, "Good bye...\n");
	fflush(stdout); // 표준 출력 스트림을 비움
	return;
}

commands make_command_token(char *command_line) // 명령어 전체 문장 토큰화
{
	commands result;
	char *tmp;
	char *command;

	result.argv = (char **)malloc(sizeof(char*) * BUFFER_SIZE);
	result.argc = 0;

	if((command = strtok(command_line, " ")) == NULL) { // 엔터만 쳤을 경우
		result.argv[result.argc] = (char *)malloc(sizeof(char));
		strcpy(result.argv[result.argc], "");
		return result;
	}

	to_lower_case(command); // 명령어 소문자화
	result.argv[result.argc] = (char *)malloc(sizeof(char) * strlen(command)); // 메모리 공간 할당
	strcpy(result.argv[result.argc++], command); // 토큰 배열에 복사

	while((tmp = strtok(NULL, " ")) != NULL) { // 나머지 인자 복사
		result.argv[result.argc] = (char *)malloc(sizeof(char) * strlen(command)); // 메모리 공간 할당
		strcpy(result.argv[result.argc++], tmp); // 토큰 배열에 복사
	}

	return result;
}

/*
int check_option(int argc, char *argv[]) // 옵션 인자 검사
{
	int c, i;
	int saved_argc;
	char **saved_argv;

	saved_argc = argc;
	saved_argv = (char **)malloc(sizeof(char *)*saved_argc);

	for(i = 0; i < saved_argc; i++)
	{
		saved_argv[i] = (char *)malloc(sizeof(char)*BUFFER_SIZE);
		strcpy(saved_argv[i],argv[i]);
		printf("argv[%d] : %s\n", i, saved_argv[i]);
	}

	while((c = getopt(saved_argc, saved_argv, "ird:l")) != -1) {
		printf("optind : %d\n", optind - 1);
		   switch(c) {
		   case 'i':
		   printf("%c처리\n", c);
		   break;
		   case 'r':
		   printf("%c처리\n", c);
		   break;
		   case 'd':
		   printf("%c처리\n", c);
		   break;
		   case 'l':
		   printf("%c처리 \n", c);
		   break;
		   case '?':
		   printf("모른다!\n");
		   return false;
		   }
	}

	for(i = 0; i < saved_argc; i++) {
		free(saved_argv[i]);
	}
	free(saved_argv);

	return true;
}
*/

int get_command_type(char *command) // COMMAND 타입 확인 및 반환
{
	// 명령어 타입 확인
	if(!strcmp(command,""))
		return false;
	else if(!strcmp(command, "delete"))
		return DELETE;
	else if(!strcmp(command, "size"))
		return SIZE;
	else if(!strcmp(command, "recover"))
		return RECOVER;
	else if(!strcmp(command, "tree"))
		return TREE;
	else if(!strcmp(command, "exit"))
		return EXIT;
	else if(!strcmp(command, "help"))
		return HELP;
	else
		return UNKNOWN;
}

void print_list_tree(file_node *head, int level, int level_check[], int is_root) // 모니터링 파일 목록 트리 출력
{
	file_node *now;
	char *file_name;
	char *tmp;

	now = head;

	while(true) {
		file_name = strtok(now->name, "/"); // 파일명 추출
		while((tmp = strtok(NULL, "/")) != NULL)
			strcpy(file_name, tmp);

		if(is_root) { // 루트 디렉토리 노드일 경우 디렉토리 이름만 출력 후 하위 파일 노드로 이동
			printf("%s\n", file_name); // 파일명 출력
			now = now->child; // 하위 파일 노드로 이동
			is_root = false; // 루트 확인 해제
			level_check[level++] = true; // 레벨 증가
			continue;
		}

		print_indent(level, level_check);

		if(now->next != NULL) 
			printf("├──%s\n", file_name);
		else
			printf("└──%s\n", file_name);

		if(S_ISDIR(now->attr.st_mode)) { // 같은 레벨의 파일이 디렉토리일 경우
			if(now->next != NULL) { // 같은 레벨의 파일들이 더 존재할 경우
				if(now->child != NULL) {
					level_check[level] = true;
					print_list_tree(now->child, level + 1, level_check, is_root);
					level_check[level] = false;
				}
			} else  // 같은 레벨의 파일들이 더 존재하지 않을 경우
				if(now->child != NULL) 
					print_list_tree(now->child, level + 1, level_check, is_root);
		}

		if(now->next != NULL)
			now = now->next;

		else break;
	}	
}

void print_indent(int level, int level_check[]) // 트리 출력 보조 함수
{
	int i;

	for(i = 1; i < level; i++) {
		if(level_check[i])
			printf("│  ");
		else
			printf("   ");
	}
}

char *rtrim(char *_str) // 문자열 오른쪽 공백 제거
{
	char tmp[MAX_BUFFER_SIZE];
	char *end;

	strcpy(tmp, _str);
	end = tmp + strlen(tmp) - 1;
	while(end != _str && isspace(*end))
		--end;

	*(end + 1) = '\0';
	_str = tmp;
	return _str;
}

char *ltrim(char *_str) // 문자열 왼쪽 공백 제거
{
	char *start = _str;

	while(*start != '\0' && isspace(*start))
		++start;
	_str = start;
	return _str;
}

void to_lower_case(char *str) // 문자열 소문자 변환
{
	int i = 0;

	while(str[i]) {
		if(str[i] >= 'A' && str[i] <= 'Z'){
			str[i] = str[i]+32;
		}
		i++;
	}
}

void init_option(void) // 옵션 확인 초기화
{
	option_i = false;
	option_r = false;
	option_d = false;
	option_l = false;
}

void print_usage(void) // 사용법 출력
{
	printf("Usage : [COMMAND]\n");
	printf("Command : \n");
	printf(" DELETE  <FILENAME> <END_TIME> [OPTION]     delete the file at the END_TIME\n");
	printf(" Option :  -i	                            delete file without moving 'trash' directory\n");
	printf("           -r                               re-confirm when the specified time, delete or not to delete\n\n");
	printf(" SIZE    <FILENAME> [OPTION]                print relative file path and size\n");
	printf(" Option :  -d  <NUMBER>                     print as much as NUMBER level\n\n");
	printf(" RECOVER <FILENAME> [OPTION]                restore deleted files to their original path\n");
	printf(" Option :  -l                               print file list in 'trash' directory sort by oldest deletion time before command execute\n\n");
	printf(" TREE                                       print a list of monitoring files in a tree format.\n");
	printf(" EXIT                                       program exit\n");
	printf(" HELP                                       print usage\n");
}

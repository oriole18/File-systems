#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <sys/shm.h> 
 #include <sys/ipc.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>

typedef struct file_inform{//파일 정보를 담음meta 역할
char in_name[BUFSIZ];
char out_name[BUFSIZ];
int divide;
int id;
int  command;  //(command=1)==>copy ,(command=2)==>list,(command=3)==>read(sum of splited file) 
int size;
}file_inform;


void split_fname(char* argv,char* in_head,char* in_ext );//파일명 argv를 받으면 '.'기준으로 in_head , 파일 형식 in_ext로 나눔
void Record(file_inform *meta);//meta.txt에 분할된 파일명 저장 및 분할 수 그리고 원본 파일 용량 표시
  int Check_Over(file_inform *meta);//분할 파일명 중복 체크 중복되면 1반환 
  void list();//meta.txt내에 저장된 분할된 파일명 용량 블럭수 출력
////////////////////////file_processing func/////////////////////////////////////

	void* work_t(void *addr){//스레드 수행 함수로 파일 분할 수행

		file_inform* meta;
		meta=(file_inform*)addr;
	//	printf("\nthread:%d\n",meta->id);

		FILE *fp;
		fp=fopen(meta->in_name,"rb");//원본 파일 이진읽기모드로 오픈
		if(fp==NULL){
		printf("\n%s:cant open\n",meta->in_name);
		exit(0);
		}
		FILE *fp_out;
		char *temp_head;
		char *temp_ext;
		char *file_out;
		int len=strlen((meta->out_name));
		 int id=(meta->id);
                  int d=meta->divide;
                 int ch;
		 int size=meta->size;
                  int r=0;	
	  if(d>size){//분할수가 원본파일 크기보다 크면 오류 발생

	printf("error: no d bigger than orginal file size\n");
	printf("server down\n");
	exit(100);
	  }	  
	//	printf("44");
		temp_head=(char*)malloc(sizeof(char)*len);
		temp_ext=(char*)malloc(sizeof(char)*len);
		file_out=(char*)malloc(sizeof(meta->out_name));
	//	printf("\n48\n");
		split_fname(meta->out_name,temp_head,temp_ext);//파일명 분할
		sprintf(file_out,"%s%d%s",temp_head,(meta->id+1),temp_ext);//분할 파일명 숫자 붙여서 재생성
		free(temp_head);
		free(temp_ext);
	
		fp_out= fopen(file_out,"wb");//분할 파일 오픈
	
		if(fp_out==NULL){
			printf("\ndont open file:%s\n",file_out);
			fclose(fp);
			free(file_out);
			exit(200);
			}
		int cf;
		int count=0;
		   fseek(fp, id*(size/d), SEEK_SET);//각스레드별 id번호 별로 포인터 위치 이동 =id*(size/d)--->ex:0번 스레드는 0번지부터 시작
		   for(int k=0;k<(size/d);k++){
		
		if((cf=fgetc(fp))==EOF) break;//파일복사중 원본파일 끝이면 바로 복사 중단
		
		   fputc(cf,fp_out);
		    }
		   if(id==(d-1)){//파일복사를 끝내도 원본파일을 다 복사하지 못하면 마지막 스레드가 남은 데이터를 복사
				if((cf)!=EOF){
					while((ch=fgetc(fp))!=EOF){
						if(ch!=EOF){
							fputc(ch,fp_out);

						}	
					}	
			}
				fclose(fp_out);
				free(file_out);
				fclose(fp);
			}
		}


	int Check_Over(file_inform *meta){//구조체 meta주소를 받아서 입력으로 들어온 분할파일명 중복 여부 확인
		FILE *fp;
		fp=fopen("meta.txt","r");
		if(fp==NULL){

		fp=fopen("meta.txt","w");
		fclose(fp);
		fp=fopen("meta.txt","r");
		}
		 char tmp;
  		 int cnt = 1;
		 char ch[BUFSIZ];
		 int res=0;
  	 while (fscanf(fp, "%c", &tmp) != EOF) {//meta파일 줄 갯수 카운팅
     	 if (tmp == '\n'){      
		cnt++;
 	  }


	fseek(fp,0,SEEK_SET);//포인터 0번지로 이동
	for(int i=0;i<cnt;i++){//줄 갯수 만큼 검사 실행

		fscanf(fp, "%s\n",ch);

                res=strcmp(ch,meta->out_name);

        	        if(res==0){//중복 되면 오류 생성 후 1반환
                	        printf("\n%s file exist in meta.txt-->other name please\n",ch);
              			  fclose(fp);
                	        return 1;
        	        }              
		}





		fclose(fp);
		return 0;
		}
	}
	void copy(file_inform *meta){//copy 기능 수행
	/////////////////////// file name 중복 검사/////
		int check;
		check=Check_Over(meta);
 		if(check==1){
			printf("\nserver down\n");
			exit(100);
		}
		
/////////////////////////////////////////////////////////////////
		file_inform **f_info;
	
		f_info=(file_inform**)malloc((meta->divide)*sizeof(file_inform*));//구조체 배열 동적 할당
		for(int p=0;p<meta->divide;p++){
	
		f_info[p]=(file_inform*)malloc(sizeof(file_inform));
		}
		////////////////////////////meta값을 동적으로 할당받은 배열 구조체에 설정
		int length_1=strlen(meta->in_name);
		int length_2=strlen(meta->out_name);
			for(int j=0;j<meta->divide;j++){

				strcpy(f_info[j]->in_name,meta->in_name);
				strcpy(f_info[j]->out_name,meta->out_name);
				f_info[j]->size=(meta->size);
				f_info[j]->divide=(meta->divide);
				f_info[j]->id=j;

				}


	pthread_t thread[meta->divide];//쓰레드 아이디 값 저장
	int value=0;
	int i=0,k=0;
	int *id=(int*)malloc(sizeof(int)*(meta->divide));	
			for(i=0;i<meta->divide;i++){//create thread
			id[i]=i;
		    value= pthread_create(&thread[id[i]],NULL,work_t,f_info[i]);
			if(value!=0){
				printf("\nerror :dont creat thread\n");
				exit(0);
				}
  			}
		for(k=0;k<meta->divide;k++){//thread join
			pthread_join(thread[k],NULL);
		}


	for(int y=0;y<meta->divide;y++){
		free(f_info[y]);
		}
		 Record(meta);//meta값 즉 분할 파일명 분할 갯수 용량 저장
			free(f_info);
			free(id);
	printf("\nfinish file copy\n");
	}

 	void Record(file_inform *meta){//meta값 저장

         FILE *fp;
	 fp=fopen("meta.txt","a");
	 fprintf(fp,"%s %d %d\n",meta->out_name,meta->size,meta->divide);
	 fclose(fp);

	}



	void list(){//meta 값에 파일명 파일용량, 분할수 출력
	FILE *fp;
                 fp=fopen("meta.txt","r");
                   char tmp;
                   int cnt = 1;
                  char ch[BUFSIZ];
                   int res=0;
		   char name[BUFSIZ];
		   int size;
		   int divide;
		   
           while (fscanf(fp, "%c", &tmp) != EOF) {//줄 갯수 카운팅
           if (tmp == '\n')
                  cnt++;
           }
          fseek(fp, 0L, SEEK_SET);//포인터 위치 초기화
	 printf("\n name     size       divide\n");
		while(!feof(fp)){//줄별로 내용 출력
			fscanf(fp, "%s %d %d", name, &size,&divide);
			if(feof(fp)) break;
	printf("%3s    %3d      %3d\n",name,size,divide); 
		}
	fclose(fp);
	}
	void read_t(file_inform *meta){//읽기모드 수행
	
	 FILE *fp;
                fp=fopen("meta.txt","r");//meta파일 열어서 분할 파일명 분할수 용량 정보를 가져옴
                 char tmp;
                 int cnt = 1;
                 char ch[BUFSIZ];
                 int res=0;
		 int kep=-1;
		 int size;
		 int divide;
         while (fscanf(fp, "%c", &tmp) != EOF) {
         if (tmp == '\n')//meta줄 갯수 카운팅
                 cnt++;
          }
         fseek(fp, 0L, SEEK_SET);
        for(int i=0;i<cnt;i++){

                 fscanf(fp, "%s %d %d\n ",ch,&size,&divide);
                res= strcmp(ch, meta->in_name);
                if(res==0){//파일명이 meta파일에 있는지 확인
                        printf("\n%s file exist\n",ch);
               	kep=1; break;
                }
        }
	if(kep==-1){//파일명 없으면 예외처리
	printf("\n%s file not exist in meta data\n",ch);
 	printf("server down\n");

        fclose(fp);
	exit(100);
	}
	fclose(fp);
	printf("%s    %d     %d\n",ch,size,divide);
	char *head;
	char *ext;
	FILE *fp_out;
	char *fp_sp;
	int temp;
	       	
	fp_out=fopen(meta->out_name,"wb");//복원파일명으로 이진파일 쓰기모드로 오픈
		if(fp_out==NULL){

			printf("error:%sfile cant open\n",meta->out_name);
		
			exit(100);
		}
	head=(char*)malloc(sizeof(meta->in_name));
	ext=(char*)malloc(sizeof(meta->in_name));
	fp_sp=(char*)malloc(sizeof(meta->in_name)*2);	
	
	split_fname(ch,head,ext);//read모드에서 ch에 저장된 분할 파일명 숫자 붙여서 재생성

		for(int i=0;i<divide;i++){
			sprintf(fp_sp,"%s%d%s",head,(i+1),ext);
			FILE *fp_in;
                         fp_in=fopen(fp_sp,"rb");//분할 파일 오픈
	
			for(int j=0;j<(size/divide);j++){//이중 for문으로 분할된 파일 내용을 읽어와서 복원될 파일에 복사 실행
			if(feof(fp_in)) break;
			temp=fgetc(fp_in);
			fputc(temp,fp_out);
				}
	
		if(i==(divide-1)){//만약 마지막 파일이 다른 파일에 비해 용량이 크면 나머지 부분 더 입력해줌
			if(feof(fp_in)==0){
				while((temp=fgetc(fp_in))!=EOF){
						
   	               		 	  fputc(temp,fp_out);

				}

			}
		}	
			fclose(fp_in);

		}
	 fclose(fp_out);		
	free(head);
	free(ext);
	free(fp_sp);
	}
	
	

	void processing(file_inform *meta){//명령어에 따라 copy,read,list함수 실행
	printf("\nstart_processing\n");
	switch(meta->command){

	case 1:printf("\ncopy proccess start\n"); copy(meta);
	      printf("\ncopy process finish\n"); break;
	case 2:printf("\nlist process start\n"); list();
	      printf("\nlist process finish\n"); break;
	case 3:printf("\nread process start\n"); read_t(meta);
	      printf("\nread process finish\n"); break;
	}       
	exit(100);
	}


	void set_meta(char **argv,file_inform *meta){//argv값을 구조체 file_inform에 저장

		
		if(strcmp(argv[1],"copy")==0){
			meta->command=1;
 	 
			sprintf(meta->in_name,"%s",argv[2]);
 	   	 sprintf(meta->out_name,"%s",argv[3]);
 	  	  meta->divide=atoi(argv[4]);

	 FILE *fp;
          char ch;
          fp=fopen(meta->in_name,"r");
   
          fseek(fp, 0, SEEK_END);    // 파일 포인터를 파일의 끝으로 이동시킴 
          meta->size=(ftell(fp));//파일 사이지 저장
          fclose(fp);

			}else if(strcmp(argv[1],"list")==0){

				meta->command=2;

				}else if(strcmp(argv[1],"read")==0){

					meta->command=3;

  					 sprintf(meta->in_name,"%s",argv[2]);
  				  sprintf(meta->out_name,"%s",argv[3]);
					}
			}



//this function is file name split head,ext...
//use method 
/*
 char *head=malloc()
 char *ext=malloc()
  split_fname(argv,head,ext)   argv--->file full name
	free(head);
	free(ext);
 */
 void split_fname(char* argv,char* in_head,char* in_ext ){파일명 argv를 '.'기준으로 head, ext로 분리
	char *out_name;
	char *in_name;
	int deli=0;
	in_name=(char*)malloc(sizeof(argv));
	sprintf(in_name,"%s",argv);

	 out_name=(char*)malloc(sizeof(in_name)+5);
        int i=0;
        int k=0;
        while(1){
                
                if(in_name[i]=='.'){
                        deli=i;
                        for(int j=i;j<(strlen(in_name)+1);j++){
                        sprintf((in_ext+k),"%c",in_name[j]);
                        k++;
                        }
                        break;
                }
                sprintf((in_head+i),"%c",in_name[i]);
                i++;
        }

        free(out_name);
        free(in_name);
 }

	int func_read_excep(int argc,char **argv){//read모드 예외처리

		if(argc!=4){//인자가 3개아니면 예외처리
		printf("%d",argc);
		printf("error_ you should input  read,in_file_name ,recovery_filename:3argument\n");
		return 1;
		}	
		return 0;
	}
	int func_copy_excep(int argc, char **argv){//copy모드 예외처리

	int count=0;

		if(argc!=5){//if command line arguments value is worng ----> exception excute
       			 printf("client error of input number\n");
      			  printf("error_you should input command input_file_name, output_file_name, divide number...\n");
       			 return 1;
        	}
///////checking file existing ///////////// 
   	FILE *fp=fopen(argv[2],"r+");
		if(fp==NULL){

		printf("No exist orginal file : %s\n",argv[2]);
		return 1;
		}
		fclose(fp);
/////////////////////////////////////////////
		int split=atoi(argv[4]);
		if(split<0||split==0){//분할수가 음수이거나,0, 문자로 들어오면 예외처리
		
		printf("error: split Value onliy bigger than Zero OR you dont input char only integer\n");
		printf("split>0,Integer split always Ok.......?\n");
		return 1;
		}
	return 0;
	}
//cechking exception//////////////////////////////////
	int checking_e(file_inform *meta,char** argv,int argc){//명령어에대한 예외처리
        int error_code=0;

       	if(strcmp(argv[1],"copy")==0){//copy에대한 예외처리
       	printf("start checkingd copy exception\n");
	error_code=func_copy_excep(argc,argv);
	printf("\n");
       
	}else if(strcmp(argv[1],"list")==0){//list예외처리
		printf("start checking list exception\n");
			if(argc!=2) {//리스트는  list명령어만 옴
				printf("error list command argument only 1\n");
				error_code=1;
			}
        }else if(strcmp(argv[1],"read")==0){//read모드 예외처리
    
	    	printf("start checking read exception\n");
		error_code=func_read_excep(argc,argv);
        }else{//copy,read,list외의 명령어가 들어올때 예외처리
	printf(" not exist command , you can only use copy,read,list\n");

	return 1;
	}
	if(error_code!=0) return 1;

		printf("\ninput is successful_ start argv ---->copy structure\n");
	

	return 0;
	}




/////////////////////under main ////////////////////////////
//
//
//
//
////////////////////////////////////////////////////////////
int main(int argc, char* argv[]) {
 

	int p_com[2];//구조체 포인터를 전달할 파이프 생성
	if(pipe(p_com)<0){
		printf("System_error:dont creat pipe\n");
		exit(200);
	}
	int p_error[2];//error발생여부 전달 파이프
	if(pipe(p_error)<0){
	printf("SYstem error: dont creat error checking pipe");
	exit(100);
	}

pid_t pid;
   pid = fork(); // child를 생성한다.
  if (pid < 0) { // 에러
    printf("[CLIENT] Fork failed.\n");
    return 1;
  } else if (pid > 0) { // parent (Server)
	 
	wait(NULL);
	char com[BUFSIZ];
	close(p_error[1]);
	read(p_error[0],com,BUFSIZ);
	if(strcmp(com,"error")==0){//client로부터 오류값이 전달되면 server 종료
	 printf("\nsever down\n");
		exit(200);
	} 
	file_inform* meta=(file_inform*)malloc(sizeof(file_inform));

        
	close(p_com[1]);
	read(p_com[0],meta,sizeof(meta));
	set_meta(argv,meta);

	processing(meta);//만약 오류가 없다면 meta 구조체 주소를 processing에게 전달
	free(meta);

	exit(300);
  }  else { // child (Client)
 	int err=0;

	file_inform* meta=(file_inform*)malloc(sizeof(file_inform));	
	err=checking_e(meta,argv,argc);//예외처리함수에 예외처리 수행 

		if(err==1){//err=1이면 서버에 error전달하고 종료
		   	close(p_error[0]);

            		write(p_error[1],"error",BUFSIZ);
			free(meta);
			printf("CLient down\n");
			exit(100);
		}
	        //그게 아니면 구조체 값 전달	
		close(p_error[0]);
		close(p_com[0]);
		write(p_error[1],"steady",BUFSIZ);
		write(p_com[1],meta,sizeof(meta));
		free(meta);	
	
  }
 
 return 0;
}


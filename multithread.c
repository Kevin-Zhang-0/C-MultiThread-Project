#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>
#include <math.h>
#include <sys/queue.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>
//#ifndef QSIZE
//#define QSIZE 8
//#endif







int QSIZE = 20;

typedef struct {
	char** data;
	unsigned count;
	unsigned head;
	int open;
	pthread_mutex_t lock;
	pthread_cond_t read_ready;
	pthread_cond_t write_ready;
} queue_t;


int init(queue_t *Q)
{
    Q->data = malloc(sizeof(char*)*QSIZE);
	Q->count = 0;
	Q->head = 0;
	Q->open = 1;
	pthread_mutex_init(&Q->lock, NULL);
	pthread_cond_init(&Q->read_ready, NULL);
	pthread_cond_init(&Q->write_ready, NULL);
	
	return 0;
}

int destroy(queue_t *Q)
{
	pthread_mutex_destroy(&Q->lock);
	pthread_cond_destroy(&Q->read_ready);
	pthread_cond_destroy(&Q->write_ready);

	return 0;
}


// add item to end of queue
// if the queue is full, block until space becomes available
int enqueue(queue_t *Q, char* item)
{
	pthread_mutex_lock(&Q->lock);
	
	while (Q->count == QSIZE && Q->open) {
        
		pthread_cond_wait(&Q->write_ready, &Q->lock);
	}
	if (!Q->open) {
		pthread_mutex_unlock(&Q->lock);
		return -1;
	}
	
	unsigned i = Q->head + Q->count;
   
	if (i >= QSIZE-1){
        
        QSIZE *= 2;
        char** temp = realloc(Q->data, sizeof(char*) * QSIZE);
       
        Q->data = temp;
        
    }
	
    //testing-kevin
    Q->data[i] = malloc(sizeof(char)*(strlen(item)+1));
    strcpy(Q->data[i],item);
    //test

	//Q->data[i] = item;
	++Q->count;
	
	pthread_cond_signal(&Q->read_ready);
	
	pthread_mutex_unlock(&Q->lock);
	
	return 0;
}


int dequeue(queue_t *Q, char* item)
{
	pthread_mutex_lock(&Q->lock);
	
	while (Q->count == 0 && Q->open) {
        
		pthread_cond_wait(&Q->read_ready, &Q->lock);
	}
	if (Q->count == 0) {
		pthread_mutex_unlock(&Q->lock);
		return -1;
	}
	
    //item = Q->data[Q->head];
    strcpy(item, Q->data[Q->head]);
    free(Q->data[Q->head]);
    //printf("i = %d queue data :%s",Q->head,item);
	--Q->count;
	++Q->head;
	if (Q->head == QSIZE) Q->head = 0;
	
	pthread_cond_signal(&Q->write_ready);
	
	pthread_mutex_unlock(&Q->lock);
	
	return 0;
}

int qclose(queue_t *Q)
{
	pthread_mutex_lock(&Q->lock);
	Q->open = 0;
    free(Q->data);
	pthread_cond_broadcast(&Q->read_ready);
	pthread_cond_broadcast(&Q->write_ready);
	pthread_mutex_unlock(&Q->lock);	

	return 0;
}
int length(queue_t *Q){
    pthread_mutex_lock(&Q->lock);
    
    int p = strlen(Q->data[Q->head]);
    pthread_mutex_unlock(&Q->lock);
    return p;
}


/* ******** 
 * Example client code for synchronized queue 
 */

struct targs {
	queue_t *Q;
	int id;
	int max;
	int wait;
};


/**
void *producer(void *A) 
{
	struct targs *args = A;
	
	int i, start, stop;
	printf("[%d] begin producing %d\n", args->id, args->max);

	start = args->id * 1000;
	stop = start + args->max;
	for (i = start; i < stop; ++i) {
		printf("[%d] Sending %d\n", args->id, i);
		if (enqueue(args->Q, i) != 0) {
			printf("[%d] Enqueue failed\n", args->id);
			return NULL;
		};
		sleep(args->wait);
	}
	
	printf("[%d] finished\n", args->id);
	
	return NULL;
}


void *consumer(void *A)
{
	struct targs *args = A;
	int i, j;
	
	printf("[%d] begin consuming %d\n", args->id, args->max);

	for (i = 0; i < args->max; ++i) {
		if (dequeue(args->Q, &j) != 0) {
			printf("[%d] Dequeue failed\n", args->id);
			return NULL;
		}
		printf("[%d] %d: Received %d\n", args->id, i, j);
		sleep(args->wait);
	}
	
	printf("[%d] finished\n", args->id);

	return NULL;	
}
**/



typedef struct{//arguments to pass into directory thread function
    queue_t *arg_d_q;
    queue_t *arg_f_q;
    char* suff;
}queues_struct;

typedef struct Node{//Node for linked list of words, there will be 1 linked list for each file
  int freq;//frequency of the word in the file
  char* word;//name of the word
  struct Node *next;
} Node;

typedef struct{//this will be used to create an array of structs, each index containing a file name and its linked list of word frequencies
  Node *llHead;//pointer to head of linked list containing words w/ frequencies in alphabetical order
  char* fileName;//name of file
} FileNode;

typedef struct{//arguments to pass into file thread function
    queue_t *file_arg_q;
    FileNode *wsdFiles;
    int wsdCounter;
    pthread_mutex_t lock;

}file_args;

int init(file_args *f){
    wsdCounter =0;
    pthread_mutex_init(&f->lock, NULL);
}

int increment(file__args *f){
    pthread_mutex_lock(&f->lock);
    f->wsdCounter = f->wsdCounter +1;
    pthread_mutex_unlock(&f->lock);
    
}



void freeList(Node* head_ref){
 
    /* deref head_ref to get the real head */
    Node* current = head_ref;
    Node* next = NULL;
 
    while (current != NULL)
    {
        next = current->next;
        free(current->word);
        free(current);
        current = next;
    }
 
    /* deref head_ref to affect the real head back
        in the caller. */
    head_ref = NULL;
}

void displayList(Node* h_ref){
    Node* current = h_ref;
    printf("DISPLAYING LIST\n");
 
    while (current != NULL)
    {
        printf("Node name: %s Frequency: %d\n",current->word,current->freq);
        current = current->next;
    }
    
}





/*
Inserts a word in alphabetical order into a linked list
word2: is a string
head2: is the a pointer to the 1st node in a linked list 
This parameter is a pointer to a pointer, 
because we are passing the address of the linked list to the function, 
and the linked list itself is a pointer to the 1st node in the list.
*/
Node * insertNode(char *word, Node *h,int size){
//pointer to a new node to be inserted into linked list
    Node *newNode = NULL;
    
//pointer to the previous node in the linked list
    Node *previous = NULL;
//pointer to the current node in the linked list
    Node *current = h;

//create a node on the heap
    newNode = malloc(sizeof(Node));
    newNode->freq = 0;
    newNode->word = NULL;
    newNode->next = NULL;

    if(h==NULL ){
        printf("head is nulasdasdasdadasdasl\n");
        h=malloc(sizeof(Node));
        h->word = malloc(sizeof(char)*(size+1));
        h->word = strncpy(h->word, word,size+1);
        h->freq = 1;
        h->next = NULL;
        free(newNode);
    }
/*
check to see if space is available
if no space on heap, malloc() will return NULL
*/
    else{
   //copy over word to the node
        newNode->word = malloc(sizeof(char)*(size+1));
        newNode->word = strncpy(newNode->word, word,size+1);
        newNode->freq = 1;
        //int x = strncmp(word, current->word,size);
        //printf(" size: %d word: %s ||curr->word: %s\n",size,word,current->word);
   //figure out where to insert in linked list
   //&& strncmp(word,current->word,size)>0
        while(current !=NULL && strncmp(word,current->word,size+1)>0){
            printf(" size: %d word: %s ||curr->word:%s \n",size,word,current->word);
      //move previous to current
            previous = current;
      //move current to next node
            current = current->next;

        }
      //insert at beginning of linked list
        if(previous==NULL){
            newNode->next = current;
            printf("new head");
            //change the address stored in head
            h = newNode;
        }
        else{
      //insert between previous and current
            previous->next = newNode;
            newNode->next = current;
        }
    }


    return h;
}




//dir checker
int isdir(char *name){
  struct stat data;
  int err = stat(name, &data);
  // should confirm err == 0
  if (err){
    perror(name);
    return 0;
  }

  if(S_ISDIR(data.st_mode)){
    return 1;
  
  }
  return 0;
}

//directory thread
//file thread
void *file_thread(void *temp){
    file_args *fstruct= (file_args *) temp;
    queue_t *f_q = (queue_t *)fstruct->file_arg_q;
    FileNode *fileArray = (FileNode *)fstruct->wsdFiles;

    Node *head = NULL;
    //head->freq = 0;
    //head->word = NULL;
    //head->next = NULL;
    
    int file_len = length(f_q);
    char*file_name = malloc(sizeof(char)* (file_len +10));
    
    //char* dir_name;
        
    dequeue(f_q,file_name);

    printf("filethread name: %s\n",file_name);

    int fdr;
    fdr = open(file_name,O_RDONLY);
    char c;
    char* buffer = malloc(sizeof(char)*10);
    int buff_max_size = 10;
    int buff_curr_size = 0;
    while(read(fdr,&c,1) != 0){
        if (buff_curr_size+5 >= buff_max_size){
            buffer = realloc(buffer,sizeof(char)*(buff_max_size*2));
      
            buff_max_size *= 2;
     
        }
        if( (isalpha(c)|| c =='-') ||  (isdigit(c)||0  ) ){
            //printf("adding character to buffer");
            buffer[buff_curr_size] = tolower(c);
            buff_curr_size+=1;
            
        }
        //space
        else if(isspace(c)){
            //printf("GOT HERE");
            //put into node
            buffer[buff_curr_size] = '\0';
            //first check if word exists in linked list
            //if yes increment count by 1
            //if not make new node and insert it in alphabetical order
            //
            //int inList = 0;

            
            Node *ptr = head;
            int orig = 1;
            //printf("word found");
            
            while(ptr != NULL){
                if (ptr->word != NULL){
                    if(strcmp(buffer, ptr->word) == 0){
                        orig = 0;
                        ptr->freq +=1;
                        //inList = 1;
                        break;
                    }
                }
                
                
                ptr = ptr->next;
            }
            
            

            if(orig){
                head = insertNode(buffer,head,buff_curr_size);
            }
            
            //printf("word: %s  \n",buffer);
            for(int i=0; i<buff_curr_size; i++){//resets word buffer to be empty
                buffer[i] = '\0';
            }
            


            buff_curr_size=0;



        }
        //punc
        else{
            continue;
        }




        //printf("%c",c);
    }
    displayList(head);

    filearray[fstruct->wsdCounter]->llHead = head;
    filearray[fstruct->wsdCounter]->file_name = file_name;

    increment(fstruct);
    
    freeList(head);
    close(fdr);
    free(buffer);
    //free(head);
    free(file_name);
    return NULL;
}
void *dir_thread(void *q){
    queues_struct *qstruct = (queues_struct *) q; 
    queue_t *d_q = (queue_t *)qstruct->arg_d_q;
    queue_t *f_q = (queue_t *)qstruct->arg_f_q;
    char *suffixX = (char *)qstruct->suff;
    //printf("dir_thread: %s\n", d_q->data[0]);
    while(d_q->count>0){
        if(d_q->count ==0){
            pthread_exit(NULL);
        }
        
        
        
        //int dir_len = strlen(d_q->data[d_q->head]);
        int dir_len = length(d_q);


        char*dir_name = malloc(sizeof(char)* (dir_len +10));
        char*original_dir_name = malloc(sizeof(char)* (dir_len +10));
        //char* dir_name;
        
        dequeue(d_q,dir_name);


        
        strcpy(original_dir_name,dir_name);
        //printf("o_dir_name:%s \n",dir_name);
        
        
        
        
        struct dirent *dp;
        DIR *dfd;
        //error if cannot open directory?
        
        dfd = opendir(dir_name);
        
        
        
        while((dp= readdir(dfd))!=NULL){
            
            strcpy(dir_name,original_dir_name);
            if(strcmp(dp->d_name,".")==0||strcmp(dp->d_name,"..")==0){
                //printf("dont actually read \n");
                continue;
            }

            //printf("f in d : %s",dp->d_name);
            dir_name = realloc(dir_name,sizeof(char) * (dir_len + strlen(dp->d_name)+ 20));
            strcat(dir_name,"/");
            strcat(dir_name,dp->d_name);
            if(isdir(dir_name)){
                
                //printf("DIRECTORY FOUND, name created is: %s\n",dir_name);
                enqueue(d_q,dir_name);
                continue;
            }
            else{
                int sufSwitch = 1;
                int sufLen = strlen(suffixX)-1;
                int fileLen = strlen(dir_name)-1;
                if(sufLen != 2){
                    for (int i=sufLen; i>1; i--){
                        if (suffixX[i] != dir_name[fileLen] && fileLen >=0){
                            sufSwitch = 0;
                            break;
                        }
                        fileLen--;
                    }
                }

                
                if (sufSwitch == 1){
                    printf("dir thread adds file to file q: %s\n",dir_name);
                    enqueue(f_q,dir_name);
                }
                
            }


        }

        closedir(dfd);

        free(original_dir_name);
        free(dir_name);
    }
    return NULL;
}



int main(int argc, char **argv){

    //printf("OQAWDJMIQOWD");
    queue_t dir_q;
    queue_t file_q;
    init(&dir_q);
    init(&file_q);

    queues_struct queues;
    queues.arg_d_q = &dir_q;
    queues.arg_f_q = &file_q;
    
    int isSuffix = 0;
    int dN = 1; //directory threads
    int fN = 1; //file threads
    int aN = 1; //analysis threads
    char *suffix = NULL; //file name suffix to look for
    pthread_t thread_id;//id for threads (idk yet)
    
    int fSize = 50;//number of files
    int wsdCounter = 0
    FileNode *WSD = malloc(sizeof(FileNode) * fSize);//initializes array of linked lists w/ file names
    file_args file_thread_input;
    file_thread_input.file_arg_q = &file_q;
    file_thread_input.wsdFiles = WSD;
    file_thread_input.wsdCounter = 0;
    init(&file_args);


    //Checking parameters
    for(int i = 1;i<argc;i++){
        
        
        //if parameter
        //printf("i is: %d\n", i);
        if(argv[i][0]== '-'){
            if (argv[i][1] == 'd' || argv[i][1] == 'f' || argv[i][1] == 'a'){
                int total = 0;
                
            
                for (int j=2; j<strlen(argv[i]); j++){
                    int power = pow(10, strlen(argv[i])-j-1);
                    //printf("power is: %d\n", power);
                    if (isdigit(argv[i][j]) == 0) return EXIT_FAILURE;
                    //printf("digit: %c\n",argv[i][j]);
                    int tempNum = argv[i][j]- '0';
                    //printf("tempNUm is: %d\n", tempNum);
                    total += (tempNum * power);
                    //power = power / 10;
                }
                if (total <= 0) return EXIT_FAILURE;
                else if (argv[i][1] == 'd') dN = total;
                else if (argv[i][1] == 'f') fN = total;
                else aN = total;
            }
            else if (argv[i][1] == 's'){
                isSuffix = 1;
                suffix = malloc(sizeof(char) * (strlen(argv[i])+1));
                //memcpy(&suffix, &argv[i][2], strlen(argv[i]));
                strcpy(suffix, argv[i]);
            }
            else{
                printf("Error reading parameters");
                return EXIT_FAILURE;
            }

            continue;
        }
        //queue
        //it is a directory
        if(isdir(argv[i])){
            enqueue(&dir_q,argv[i]);
            
            
        }
        //not a directory
        else{
            enqueue(&file_q,argv[i]);
            //add to file to file queue
        }
        
    }
    if (isSuffix == 0){
        suffix = malloc(sizeof(char) * 5);
        strcpy(suffix, ".txt");
    }
    queues.suff = suffix;
    
    /**
    for (int i=0; i<dir_q.count; i++){
        printf("Directory is: %s\n", dir_q.data[i]);
        
        pthread_create(&thread_id,NULL,dir_thread,&dir_q);
        pthread_join(thread_id, NULL);
    }
    
    while(dir_q.count>0){
        pthread_create(&thread_id,NULL,dir_thread,&queues);
        pthread_join(thread_id, NULL);
    }
    **/
    //unsigned int ids[dN];
    pthread_t tids[dN];
    for(int i =0;i<dN;i++){
        pthread_create(&tids[i],NULL,dir_thread,&queues);
       
    }
    for(int i =0;i<dN;i++){
        
        pthread_join(tids[i], NULL);
    }

    

    while(file_q.count>0){
        //printf("File is: %s\n", file_q.data[i]);
        
        pthread_create(&thread_id,NULL,file_thread,&file_thread_input);
        pthread_join(thread_id, NULL);
    }

    
    
    printf("dN is: %d\n", dN);
    printf("fN is: %d\n", fN);
    printf("aN is: %d\n", aN);
    printf("sN is: %s\n", suffix);
    //temp
    
    //char* printme = malloc(sizeof(char) * 100);
   

   
  
    free(WSD);

    free(suffix);
    qclose(&dir_q);
    qclose(&file_q);

    return EXIT_SUCCESS;
}
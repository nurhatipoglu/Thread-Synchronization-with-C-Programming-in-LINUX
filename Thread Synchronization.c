/*
derleme: gcc part1.c -lpthread
çalıştırma: ./a.out
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#define K 10
#define N 50
int loopn=0;
int loopk=0;

enum{okuma, yazma} dosyacond = yazma;
int thread_live=1;

pthread_t threads[N];
int condlst2[N];
pthread_mutex_t mutex;
pthread_cond_t condlst;

struct message_data{
  int thread_id;
  int *message;
};
struct node{
  int initial;
  struct node *left, *right;
};
struct node greaterfinder[K];
struct node* createnode(int arg){
  struct node *newnode = (struct node*)malloc(sizeof(struct node));
  newnode->initial=arg;
  //buyukse agacın sağ koluna eklenir, küçükse sol koluna eklenir.
  newnode->left=NULL; 
  newnode->right=NULL;
  return(newnode);
}
void insertNode(struct node *currentNode,struct node *addedNode)
{
     if(addedNode-> initial<currentNode ->initial)
     {

          if(currentNode->left == NULL)
          {

               currentNode->left = addedNode;
          }
          else
          {

          insertNode(currentNode->left,addedNode);
          }

     }
     else{
          if(currentNode->right==NULL)
          {
               currentNode->right=addedNode;
          }
          else{
               insertNode(currentNode->right,addedNode);
          }
     }
   }

void insert(struct node *root,int data)
{
     struct node *temp=createnode(data);
     if(!root)
          root=&temp;
     else{
          insertNode(root,temp);
     }
}


   int greater(struct node *root){
     while(root !=NULL && root->right !=NULL){
       root=root->right; //kucukten buyuge sıralıyor. 
     }
     return root->initial; //en son elemanı yani max elemanı döndürür.
   }

void* fthread(void* arg_msg){
  struct message_data *msg;//mesjı alır
  msg = (struct message_data *) arg_msg; //tip dönüşümü gerçekleşir.
  int taskid = msg->thread_id; //0,1,2,...N degerleri alır.
  char dosya_ismi[20];
  sprintf( dosya_ismi,"%d",taskid);//0,1,2,..N değerleriyle dosya adları oluşturuyor
  while(thread_live){

    pthread_mutex_lock(&mutex);//kilidin içine giren thread çalışmaya başlar
    while(!condlst2[taskid]){
      pthread_cond_wait(&condlst,&mutex);//durum değişkeni 0 ise thread bekletilir.
    }
    if(!dosyacond){
      FILE *fp2;
      if ((fp2=fopen(dosya_ismi, "rb")) == NULL) {
          printf("Dosya açılamadı!\n");
          exit(1);
      }
      int value;
      for(int u=0;u<K;u++){
       fscanf(fp2,"%d\n",&value);//k adet değer dosyadan bir thread yardımıyla okunur
       if(loopk==u){
         printf("%d.dosyadan %d . deger okundu:%d\n",taskid,u+1,value);//degeri bastırırız
         break;
       }
      }
      fclose(fp2);
      insert(&greaterfinder[loopk],value);//agaca eleman ekler.
     if(taskid==N-1){ //thread in sonuna geldiysek
       //ana thread çalışsın diğerleri uyusun.
        condlst2[taskid]=0; 
        condlst2[0]=1;
        /*işlem*/
        pthread_mutex_unlock(&mutex);//critical section dan çıkış sağlanır.
        loopk++; //dosyadan degerleri okurken thread i uyutup uyandırma yapıyoruz
        if(loopk==K){
        thread_live=0;//join komutunun thread i öldürmemesi için kullanılır.
        for(int j=0;j<K;j++){
        printf("###%d.dosya icin: %d###\n",j+1,greater(&greaterfinder[j]));
        }
      }
      }
      if(taskid!=N-1){
        //eğer son değer değilse bir sonraki threadin değerini 1 yaparak aktif ediyoruz.
        condlst2[taskid]=0;
        condlst2[taskid+1]=1;
        pthread_mutex_unlock(&mutex);
      }
          pthread_cond_broadcast(&condlst);//hepsini uyandırır.
    }


    if(dosyacond){
      FILE *fp;
      if ((fp=fopen(dosya_ismi, "a")) == NULL) {
          printf("Dosya açılamadı!\n");
          exit(1);
      }
      for(int u=0;u<K;u++){
       fprintf(fp,"%d\n",(msg->message)[u]);//dosyaya değerleri yazıyoruz.
      }
      fclose(fp);

      if(taskid==N-1){
        condlst2[taskid]=0;
        condlst2[0]=1;
        dosyacond=okuma;
        printf("####yazmabitti#####\n");
        pthread_mutex_unlock(&mutex);//yazdıktan sonra thread çıkış yaptı
        pthread_cond_broadcast(&condlst);//uyandırdık.
      }
      if(taskid!=N-1){
        condlst2[taskid]=0;
        condlst2[taskid+1]=1;
        pthread_mutex_unlock(&mutex);
        pthread_cond_broadcast(&condlst);
      }

    }

  }
}



int main(int argc, char const *argv[]) {
  for(int i=0;i<N-1;i++){
    condlst2[i+1]=0;
  }
  condlst2[0]=1;
  srand(time(NULL));
  if(pthread_mutex_init(&mutex,NULL)) //mutex oluşturulur.
    perror("mutex_olusturma_hatasi!");

  if(pthread_cond_init(&condlst,NULL)) //durum değişkeni oluşturuluyor
    perror("condition_olusturma_hatasi!");

  for(int i=0;i<N;i++){
    int *msg_ptr = malloc(sizeof(int)*K); //dinamik bellek lanı oluşturulur.
    for(int y=0;y<K;y++){
    int f=rand()%100;//0 ile 100 arası rasgele degerler olusturulur.
    msg_ptr[y]=f;//degerler msg dizisine atanır.
    }

    struct message_data thread_data_array;//iki değişkenli struct yapısı oluşturulur
    thread_data_array.thread_id = i; //0,1,2,3..N şeklinde değerler.
    thread_data_array.message = msg_ptr;//rasgele değerleri tutar.



    printf("%d.Thread oluşturuluyor....\n", i+1);
    if(pthread_create(&threads[i], NULL, fthread, (void *)&thread_data_array)) //id ve rasgele degerlerle fthread e gider
    {
    printf("hata%d",i);
    exit(1);
    }
    sleep(1);//uyku durumuna sokar.
    }
    for(int i=0;i<N;i++)
    {
      pthread_join(threads[i],NULL);
    }
    printf("########programınsonu#######\n");


  return 0;
}
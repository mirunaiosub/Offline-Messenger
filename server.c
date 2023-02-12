#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sqlite3.h>

/* portul folosit */
#define PORT 2728
 
/* eroare returnata de unele apeluri */
extern int errno;

static void *treat(void *); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */
 
sqlite3 *db;
typedef struct thData
{
 int idThread; //id-ul thread-ului tinut in evidenta de acest program
 int cl;       //descriptorul intors de accept
 sqlite3 *db; 
} thData;
 
 
int main()
{
 struct sockaddr_in server; // structura folosita de server
 struct sockaddr_in from;
 int sd; //descriptorul de socket
 pthread_t th[100]; //Identificatorii thread-urilor care se vor crea
 int i = 0;
 
 /* crearea unui socket */
 if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
 {
   perror("[server]Socket ERROR.\n");
   return errno;
 }
 /* utilizarea optiunii SO_REUSEADDR */
 int on = 1;
 setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
 
 /* pregatirea structurilor de date */
 bzero(&server, sizeof(server));
 bzero(&from, sizeof(from));
 
 /* umplem structura folosita de server */
 /* stabilirea familiei de socket-uri */
 server.sin_family = AF_INET;
 /* acceptam orice adresa */
 server.sin_addr.s_addr = htonl(INADDR_ANY);
 /* utilizam un port utilizator */
 server.sin_port = htons(PORT);
 
 /* atasam socketul */
 if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
 {
   perror("[server]Eroare la bind().\n");
   return errno;
 }
 
 /* punem serverul sa asculte daca vin clienti sa se conecteze */
 if (listen(sd, 2) == -1)
 {
   perror("[server]Eroare la listen().\n");
   return errno;
 }

   printf("[server]Waiting at port %d...\n", PORT);
   fflush(stdout);
 
 /* servim in mod concurent clientii...folosind thread-uri */
sqlite3_open("database.db", &db);
 while (1)
 {
   int client;
   thData *td; //parametru functia executata de thread
   int length = sizeof(from);
 
   /* acceptam un client (stare blocanta pina la realizarea conexiunii) */
   if ((client = accept(sd, (struct sockaddr *)&from, &length)) < 0)
   {
     perror("[server]Accept error.\n");
     continue;
   }
 
   /* s-a realizat conexiunea, se astepta mesajul */

   td = (struct thData *)malloc(sizeof(struct thData));
   td->idThread = i++;
   td->cl = client;
   td->db = db;
 
   pthread_create(&th[i], NULL, &treat, td);
 
 } 
};

static void *treat(void * arg)
{
 struct thData tdL;
 tdL = *((struct thData *)arg);
 printf("[Thread %d] Waiting for a message...\n", tdL.idThread);
 fflush(stdout);
 pthread_detach(pthread_self());
 raspunde((struct thData*) arg);
 return (NULL);
};

void raspunde(void *arg)
{
  struct thData tdL;
  tdL = *((struct thData*)arg);
  short int rc;
  int isLogged=0;
  char username[1000];
  char receivedC[1000];
  char sendC[1000];
  char password[1000];
  const char sql[300];
  char *p; /*pentru spargerea in cuvinte*/
  char s[2]=" ";/*separatorii*/
  while(1)
  {
  if (read(tdL.cl, receivedC, sizeof(receivedC)) <= 0)
   {
     printf("[Thread %d]\n", tdL.idThread);
     perror("READ ERROR from client.\n");
   }
  
   /* returnam mesajul clientului */
    if (strncmp(receivedC, "login ", 6) == 0)
    {
      if(isLogged == 0)
      {
          p=strtok(receivedC, s);
          p=strtok(0,s);
          strcpy(username, p);
          p=strtok(0,s);
          strcpy(password, p);
          int id=0;
          sqlite3_stmt *stmt;
          /*const char sql[300];*/
          sprintf(sql, "SELECT ID, isLogged FROM users where name='%s' and password='%s';", username, password);
  
          int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
          if (rc != SQLITE_OK) {
            printf("err: solved from database! ", sqlite3_errmsg(db));
            return;
            }
          while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
              id = sqlite3_column_int (stmt, 0);
              isLogged = sqlite3_column_int (stmt, 1);
          }
          if (rc != SQLITE_DONE) {
            printf("err: solved from database! ", sqlite3_errmsg(db));
          }
          sqlite3_finalize(stmt);
          if (id == 0)
          {
            sprintf(sendC, "User '%s' does not exist or wrong password!", username);
          }
          else
          {
            sprintf(sql, "UPDATE USERS set isLogged=1 where id='%i' and isLogged=0;", id);
            int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
              if (rc != SQLITE_OK) {
            printf("err: solved from database! ", sqlite3_errmsg(db));
            return;
            }
            rc = sqlite3_step(stmt);
            isLogged = 1;
            if (rc != SQLITE_DONE) {
              printf("err: solved from database! ", sqlite3_errmsg(db));
            }
            sqlite3_finalize(stmt);
            sprintf(sendC, "You've logged in, your ID is: '%i'", id);
          }
        
      }
      else
          sprintf(sendC, "You are already logged in!");
    }
 
    else if(strncmp(receivedC, "logout ", 7) == 0)
    {
      if(isLogged == 1)
      {  
          isLogged=0;
          p=strtok(receivedC, s);
          p=strtok(0,s);
          strcpy(username, p);
  
          int id=0;
          sqlite3_stmt *stmt;
          sprintf(sql, "SELECT ID, isLogged FROM users where name='%s';", username);
  
          int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
          if (rc != SQLITE_OK) {
            printf("err: solved from database! ", sqlite3_errmsg(db));
            return;
            }
          while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
              id = sqlite3_column_int (stmt, 0);
              isLogged = sqlite3_column_int (stmt, 1);
          }
          if (rc != SQLITE_DONE) {
            printf("err: solved from database! ", sqlite3_errmsg(db));
          }
          sqlite3_finalize(stmt);
          if (id == 0)
            sprintf(sendC, "User '%s' does not exist!", username);
          else
          {
            sprintf(sql, "UPDATE USERS set isLogged=0 where id='%i' and isLogged=1;", id);
            int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
            if (rc != SQLITE_OK) {
            printf("err: solved from database! ", sqlite3_errmsg(db));
            return;
            }
            rc = sqlite3_step(stmt);
            if (rc != SQLITE_DONE) {
              printf("err: solved from database! ", sqlite3_errmsg(db));
            }
            sqlite3_finalize(stmt);
            isLogged = 0;
            sprintf(sendC, "You've been logged out!");
          }
      }
      else
          sprintf(sendC, "You have to be logged in first in order to log out!");
    }
 
    else if (strncmp(receivedC, "register ", 9) == 0)
    {
        if(isLogged == 0)
        {
          int id = 0;
          p=strtok(receivedC, s);
          p=strtok(0,s);
          strcpy(username, p);
          p=strtok(0,s);
          strcpy(password, p);
  
          
          sqlite3_stmt *stmt;
  
          sprintf(sql, "SELECT ID FROM users where name='%s';", username);
  
          int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
          if (rc != SQLITE_OK) {
            printf("err: solved from database! ", sqlite3_errmsg(db));
            return;
            }
          while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
              id = sqlite3_column_int (stmt, 0);
          }
          if (rc != SQLITE_DONE) {
            printf("err: solved from database! ", sqlite3_errmsg(db));
          }
          sqlite3_finalize(stmt);
          if (id == 0)
          {
            sprintf(sql, "INSERT INTO users values(NULL,'%s','%s',0);", username, password);
            int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
            if (rc != SQLITE_OK) {
              printf("err: solved from database! ", sqlite3_errmsg(db));
              return;
            }
            while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
                id = sqlite3_column_int (stmt, 0);
              }    
            if (rc != SQLITE_DONE) {
                printf("err: solved from database! ", sqlite3_errmsg(db));
            }
            sqlite3_finalize(stmt);
            sprintf(sendC, "User '%s' successfully created, now you can log in!", username);
          }
          else
            sprintf(sendC, "User %s already exists!", username);
        }
        else
            sprintf(sendC, "You have to be logged out in order to register!");
    }
 
    else if (strncmp(receivedC, "message ", 8) == 0)
    {
      if (isLogged == 0)
      {
        sprintf(sendC, "You must be logged in if you want to message someone!");
      }
      else
      {
        char receiver[100];
        char message[1000];
        sqlite3_stmt *stmt;
  
        p=strtok(receivedC, s);
        p=strtok(0, s);
        strcpy(receiver, p);
        p=strtok(0, "'");
        strcpy(message, p);
        int idr;
        sprintf(sql, "SELECT id FROM users where name='%s';", receiver);
        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
            printf("err: solved from database! ", sqlite3_errmsg(db));
            return;
            }
          while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
              idr = sqlite3_column_int (stmt, 0);
          }
          if (rc != SQLITE_DONE) {
            printf("err: solved from database! ", sqlite3_errmsg(db));
          }
          sqlite3_finalize(stmt);
        if (idr == 0)
        {
          sprintf(sendC, "User '%s' does not exist!", receiver);
        }
        else
        {
          sprintf(sql, "INSERT INTO MESSAGES VALUES(NULL, '%s','%s','%s',0, 1, 0);", username, message, receiver);
          int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
          if (rc != SQLITE_OK) {
            printf("err: solved from database! ", sqlite3_errmsg(db));
            return;
            }
          if((rc = sqlite3_step(stmt)) != SQLITE_ROW) {
              printf("err: solved from database! ", sqlite3_errmsg(db));
          }
          if (rc != SQLITE_DONE) {
            printf("err: solved from database! ", sqlite3_errmsg(db));
          }
          sqlite3_finalize(stmt);
          sprintf(sendC, "Message successfully sent!");
        }
      }
    }
 
    else if (strncmp(receivedC, "delete ", 7) == 0)
    {
        if (isLogged == 0)
        {
          sprintf(sendC, "You must be logged in if you want to delete a message!");
        }
        else
        {
          char receiver[100];
          char message[1000];
          sqlite3_stmt *stmt;

          p=strtok(receivedC, s);
          p=strtok(0, s);
          strcpy(receiver, p);
          p=strtok(0, "'");
          strcpy(message, p);
          int reply, idM;
          sprintf(sql, "SELECT hasReply, ID FROM MESSAGES where receiver='%s';", receiver);
          int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
          if (rc != SQLITE_OK) {
              printf("err: solved from database! ", sqlite3_errmsg(db));
              return;
              }
            while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
                reply = sqlite3_column_int (stmt, 0);
                idM = sqlite3_column_int (stmt, 1);
            }
            if (rc != SQLITE_DONE) {
              printf("err: solved from database! ", sqlite3_errmsg(db));
            }
            sqlite3_finalize(stmt);
          if (idM == 0)
            sprintf(sendC, "User '%s' & message do not exist!", receiver);
          else if(reply == 1)
            sprintf(sendC, "The receiver already replied, you can't delete the message anymore!");
          else
          {
            sprintf(sql, "DELETE FROM MESSAGES WHERE receiver='%s' and message= '%s';", receiver, message);
            int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
            if (rc != SQLITE_OK) {
              printf("err: solved from database! ", sqlite3_errmsg(db));
              return;
              }
            if((rc = sqlite3_step(stmt)) != SQLITE_ROW) {
                printf("err: solved from database! ", sqlite3_errmsg(db));
            }
            if (rc != SQLITE_DONE) {
              printf("err: solved from database! ", sqlite3_errmsg(db));
            }
            sqlite3_finalize(stmt);
            sprintf(sendC, "Message successfully deleted!");
          }
        }
    }

    else if (strncmp(receivedC, "reply ", 6) == 0)
    {
        if (isLogged == 0)
        {
          sprintf(sendC, "You must be logged in!");
        }
        else 
        {
        char receiver[100];
        char message[1000];
        sqlite3_stmt *stmt;
  
        p=strtok(receivedC, s);
        p=strtok(0, s);
        strcpy(receiver, p);
        p=strtok(0, "'");
        strcpy(message, p);
        int id=0;
        sprintf(sql, "SELECT ID FROM MESSAGES where sender='%s' and receiver='%s';", receiver, username);
        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
            printf("err: solved from database! ", sqlite3_errmsg(db));
            return;
            }
          while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
              id = sqlite3_column_int (stmt, 0);
          }
          if (rc != SQLITE_DONE) {
            printf("err: solved from database! ", sqlite3_errmsg(db));
          }
          sqlite3_finalize(stmt);
        if (id == 0)
        {
          sprintf(sendC, "User '%s' does not exist or you have no messages from him/her!", receiver);
        }
        else 
        {
          sprintf(sql, "INSERT INTO MESSAGES VALUES(NULL, '%s','%s','%s','%i', 1, 0);", username, message, receiver, id);
          int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
          if (rc != SQLITE_OK) {
            printf("err: solved from database! ", sqlite3_errmsg(db));
            return;
            }
          if((rc = sqlite3_step(stmt)) != SQLITE_ROW) {
              printf("err: solved from database! ", sqlite3_errmsg(db));
          }
          if (rc != SQLITE_DONE) {
            printf("err: solved from database! ", sqlite3_errmsg(db));
          }
          sqlite3_finalize(stmt);

          sprintf(sql, "UPDATE MESSAGES set NEW=0, hasReply=1 where sender='%s' and receiver='%s';", receiver, username);
          rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
          if (rc != SQLITE_OK) {
            printf("err: solved from database! ", sqlite3_errmsg(db));
            return;
            }
          if((rc = sqlite3_step(stmt)) != SQLITE_ROW) {
              printf("err: solved from database! ", sqlite3_errmsg(db));
          }
          if (rc != SQLITE_DONE) {
            printf("err: solved from database! ", sqlite3_errmsg(db));
          }
          sqlite3_finalize(stmt);
          sprintf(sendC, "You succesfully replied!");
        }
        }
    }

    else if (strncmp(receivedC, "check new", 9) == 0)
    {
        if (isLogged == 0)
        {
          sprintf(sendC, "You must be logged in to check messages!");
        }
        else
        {
        char *sender[100], *receiver[100], message[1000]={0};
        sqlite3_stmt *stmt;
        int id=0;
        sprintf(sql, "SELECT ID FROM MESSAGES where receiver='%s' and NEW=1;", username);
        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
            printf("err: solved from database! ", sqlite3_errmsg(db));
            return;
            }
        rc = sqlite3_step(stmt);
        id = sqlite3_column_int (stmt, 0);
    
        if (rc != SQLITE_DONE) {
            printf("err: solved from database! ", sqlite3_errmsg(db));
        }
        sqlite3_finalize(stmt);

        if (id == 0)
        {
          sprintf(sendC, "User '%s' does not have any new messages!", username);
        }
        else  
        {
          sprintf(sql, "SELECT sender, message from MESSAGES where receiver='%s' and NEW=1;", username);
          rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
          if (rc != SQLITE_OK) {
            printf("err: solved from database! ", sqlite3_errmsg(db));
            return;
            }
          while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
                strcat(message, "You have a new message from ");
                strcat(message, sqlite3_column_text (stmt, 0));
                strcat(message, ": ");
                strcat(message, sqlite3_column_text (stmt, 1));
                strcat(message, "\n");
          }
          sprintf(sendC, "New messages: \n%s \n", message);
          if (rc != SQLITE_DONE) {
            printf("err: solved from database! ", sqlite3_errmsg(db));
          }
          sqlite3_finalize(stmt);

          sprintf(sql, "UPDATE MESSAGES set NEW=0 where receiver='%s';", username);
          rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
          if (rc != SQLITE_OK) {
            printf("err: solved from database! ", sqlite3_errmsg(db));
            return;
            }
          if((rc = sqlite3_step(stmt)) != SQLITE_ROW) {
              printf("err: solved from database! ", sqlite3_errmsg(db));
          }
          if (rc != SQLITE_DONE) {
            printf("err: solved from database! ", sqlite3_errmsg(db));
          }
          sqlite3_finalize(stmt);

          
        }
        }
    }

    else if (strncmp(receivedC, "check ", 6) == 0)
    {
        if (isLogged == 0)
        {
          sprintf(sendC, "You must be logged in to check history with someone!");
        }
        else
        {
        char person[100], message[1000]={0};
        sqlite3_stmt *stmt;
        int id=0;
  
        p = strtok(receivedC, s);
        p = strtok(0, s);
        strcpy(person, p);
  
        sprintf(sql, "select id from messages where (sender='%s' and receiver='%s') or (sender='%s' and receiver='%s');", person, username, username, person);
        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
            printf("err: solved from database! ", sqlite3_errmsg(db));
            return;
            }
        rc = sqlite3_step(stmt);
        id = sqlite3_column_int (stmt, 0);
    
        if (rc != SQLITE_DONE) {
            printf("err: solved from database! ", sqlite3_errmsg(db));
        }
        sqlite3_finalize(stmt);
  
        if (id == 0)
        {
          sprintf(sendC, "You have no conversation with user '%s'.", person);
        }
        else 
        {
          sprintf(sql, "select sender, message from messages where (sender='%s' and receiver='%s') or (sender='%s' and receiver='%s' and NEW=0);", person, username, username, person);
          rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
          if (rc != SQLITE_OK) {
            printf("err: solved from database! ", sqlite3_errmsg(db));
            return;
            }
          while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
                strcat(message, sqlite3_column_text (stmt, 0));
                strcat(message, ": ");
                strcat(message, sqlite3_column_text (stmt, 1));
                strcat(message, "\n");
          }
          sprintf(sendC, "Your conversation: \n%s \n", message);
          if (rc != SQLITE_DONE) {
            printf("err: solved from database! ", sqlite3_errmsg(db));
          }
          sqlite3_finalize(stmt);
  
        }
        }
    }


    else if (strncmp(receivedC, "quit", 4) == 0)
    {
      close(tdL.cl);
      return NULL;
    }
    else if (strncmp(receivedC, "menu", 4) == 0)
    {
      sprintf(sendC, "This is the menu: \n register <username> <password> - every new user it must be registered to the database. \n login - Each user must be registered in order to log in. First, we check if it exists in the database. \n logout - option to log out. \n message <receiver> - option to send a message. \n check new - users are able to check new messages. \n check <person> - online users can check a conversation with a certain person. \n delete - users are able to delete a message from any conversation, but only as long as the receiver hasn’t replied yet. \n quit - users are able to disconnect from the server. \n");
    }

    else 
    {
      sprintf(sendC, "This is the menu: \n register <username> <password> - every new user it must be registered to the database. \n login - Each user must be registered in order to log in. First, we check if it exists in the database. \n logout - option to log out. \n message <receiver> - option to send a message. \n check new - users are able to check new messages. \n check <person> - online users can check a conversation with a certain person. \n delete - users are able to delete a message from any conversation, but only as long as the receiver hasn’t replied yet. \n quit - users are able to disconnect from the server. \n");
    }
     if (write(tdL.cl, sendC, sizeof(sendC)) <= 0)
     {
       printf("[Thread %d] ", tdL.idThread);
       perror("[Thread] WRITE ERROR to client.\n");
     }
     else
       printf("[Thread %d] The message has been successfully sent.\n", tdL.idThread);
  }
  return NULL;
  
}

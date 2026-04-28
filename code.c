#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RST "\033[0m"
#define GRN "\033[0;32m"
#define RED "\033[0;31m"
#define CYN "\033[0;36m"
#define YLW "\033[1;33m"

#define AFILE "accounts.dat"
#define TFILE "trans.dat"
#define PINLEN 6
#define ADMINPW "admin123"

#ifdef _WIN32
  #define CLR() system("cls")
#else
  #define CLR() printf("\033[2J\033[H")
#endif

typedef struct { long id; char name[50], pin[PINLEN]; double bal; int active; } Acc;
typedef struct { long id; char type[20]; double amt, bal; } Tx;

/* ── Utilities ── */
void flush()          { int c; while ((c=getchar())!='\n'&&c!=EOF); }
void divider()        { printf(CYN"====================================\n"RST); }
void header(char *t)  { CLR(); divider(); printf(YLW"      SIMPLE BANKING SYSTEM\n"RST); printf(CYN"            %s\n"RST,t); divider(); }
void pause()          { printf("\nPress Enter..."); getchar(); }
void readline(char *b, int n) { fgets(b,n,stdin); b[strcspn(b,"\n")]=0; }

/* ── File Ops ── */
long nextID() {
    FILE *f=fopen(AFILE,"rb"); Acc a; long m=100000;
    if(!f) return m+1;
    while(fread(&a,sizeof a,1,f)) if(a.id>m) m=a.id;
    fclose(f); return m+1;
}

void saveAcc(Acc a)   { FILE *f=fopen(AFILE,"ab"); if(f){fwrite(&a,sizeof a,1,f);fclose(f);} }

int loadAcc(long id, Acc *a) {
    FILE *f=fopen(AFILE,"rb"); Acc t;
    if(!f) return 0;
    while(fread(&t,sizeof t,1,f)) if(t.id==id&&t.active){*a=t;fclose(f);return 1;}
    fclose(f); return 0;
}

int updateAcc(Acc a) {
    FILE *f=fopen(AFILE,"r+b"); Acc t;
    if(!f) return 0;
    while(fread(&t,sizeof t,1,f))
        if(t.id==a.id){fseek(f,-(long)sizeof t,SEEK_CUR);fwrite(&a,sizeof a,1,f);fclose(f);return 1;}
    fclose(f); return 0;
}

void logTx(long id, char *type, double amt, double bal) {
    FILE *f=fopen(TFILE,"ab");
    if(!f) return;
    Tx t={id,"",amt,bal}; strcpy(t.type,type);
    fwrite(&t,sizeof t,1,f); fclose(f);
}

/* ── Features ── */
void createAccount() {
    Acc a={0}; header("OPEN ACCOUNT");
    printf("Full name: "); readline(a.name,50);
    printf("4-digit PIN: "); readline(a.pin,PINLEN);
    printf("Opening deposit: "); scanf("%lf",&a.bal); flush();
    if(a.bal<0){printf(RED"Invalid amount.\n"RST);return;}
    a.id=nextID(); a.active=1;
    saveAcc(a); logTx(a.id,"OPEN",a.bal,a.bal);
    printf(GRN"\nAccount created! Your number: %ld\n"YLW"Save this number to log in.\n"RST,a.id);
}

int login(Acc *a) {
    long id; char pin[PINLEN]; header("LOG IN");
    printf("Account number: "); scanf("%ld",&id); flush();
    if(!loadAcc(id,a)){printf(RED"Account not found.\n"RST);return 0;}
    printf("PIN: "); readline(pin,PINLEN);
    if(strcmp(pin,a->pin)){printf(RED"Wrong PIN.\n"RST);return 0;}
    printf(GRN"\nWelcome back, %s!\n"RST,a->name); return 1;
}

void deposit(Acc *a) {
    double amt; printf("Deposit amount: "); scanf("%lf",&amt); flush();
    if(amt<=0){printf(RED"Invalid amount.\n"RST);return;}
    a->bal+=amt; updateAcc(*a); logTx(a->id,"DEPOSIT",amt,a->bal);
    printf(GRN"New balance: %.2f\n"RST,a->bal);
}

void withdraw(Acc *a) {
    double amt; printf("Withdraw amount: "); scanf("%lf",&amt); flush();
    if(amt<=0||amt>a->bal){printf(RED"Invalid amount or insufficient funds.\n"RST);return;}
    a->bal-=amt; updateAcc(*a); logTx(a->id,"WITHDRAW",amt,a->bal);
    printf(GRN"New balance: %.2f\n"RST,a->bal);
}

void transfer(Acc *a) {
    long to; double amt; Acc rec;
    printf("Recipient account: "); scanf("%ld",&to); flush();
    if(to==a->id){printf(RED"Can't transfer to yourself.\n"RST);return;}
    if(!loadAcc(to,&rec)){printf(RED"Recipient not found.\n"RST);return;}
    printf("Amount: "); scanf("%lf",&amt); flush();
    if(amt<=0||amt>a->bal){printf(RED"Invalid amount or insufficient funds.\n"RST);return;}
    a->bal-=amt; rec.bal+=amt;
    updateAcc(*a); updateAcc(rec);
    logTx(a->id,"TRF-OUT",amt,a->bal); logTx(rec.id,"TRF-IN",amt,rec.bal);
    printf(GRN"Sent %.2f to %s. Your balance: %.2f\n"RST,amt,rec.name,a->bal);
}

void history(long id) {
    FILE *f=fopen(TFILE,"rb"); Tx t; int found=0;
    header("TRANSACTION HISTORY");
    printf("%-14s | %10s | %s\n","Type","Amount","Balance After"); divider();
    while(f&&fread(&t,sizeof t,1,f))
        if(t.id==id){printf("%-14s | %10.2f | %.2f\n",t.type,t.amt,t.bal);found=1;}
    if(!found) printf("No transactions yet.\n");
    if(f) fclose(f);
}

void dashboard(Acc *a) {
    int ch;
    while(1) {
        header("MY ACCOUNT");
        printf("Hello, %s  |  Balance: %.2f\n\n",a->name,a->bal);
        printf("1. Check Balance\n2. Deposit\n3. Withdraw\n4. Transfer\n5. History\n0. Log Out\n\nChoice: ");
        scanf("%d",&ch); flush();
        switch(ch) {
            case 1: printf(CYN"Balance: %.2f\n"RST,a->bal); break;
            case 2: deposit(a);   break;
            case 3: withdraw(a);  break;
            case 4: transfer(a);  break;
            case 5: history(a->id); break;
            case 0: printf(YLW"Logged out. Goodbye!\n"RST); return;
            default: printf(RED"Invalid option.\n"RST);
        }
        pause();
    }
}

void adminPanel() {
    FILE *f=fopen(AFILE,"rb"); Acc a;
    header("ADMIN PANEL");
    printf("%-12s | %-22s | %s\n","Account No.","Name","Balance"); divider();
    while(f&&fread(&a,sizeof a,1,f))
        if(a.active) printf("%-12ld | %-22s | %.2f\n",a.id,a.name,a.bal);
    if(f) fclose(f);
    pause();
}

/* ── Main ── */
int main() {
    int ch; Acc user;
    while(1) {
        header("MAIN MENU");
        printf("1. Log In\n2. Open Account\n3. Admin Panel\n0. Exit\n\nChoice: ");
        scanf("%d",&ch); flush();
        switch(ch) {
            case 1: if(login(&user)) dashboard(&user); else pause(); break;
            case 2: createAccount(); pause(); break;
            case 3: {
                char pw[20]; printf("Admin password: "); readline(pw,20);
                if(!strcmp(pw,ADMINPW)) adminPanel();
                else { printf(RED"Wrong password.\n"RST); pause(); }
                break;
            }
            case 0: printf(GRN"Thank you! Goodbye.\n"RST); return 0;
            default: printf(RED"Invalid option.\n"RST); pause();
        }
    }
}

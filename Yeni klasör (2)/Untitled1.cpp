#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <omp.h>
#include <conio.h>
#include <clocale>

#define NUM_TASKS 16
#define NUM_ITERATIONS 200
#define PROGRESS_BAR_HEIGHT 20

// G�revlerin tamamlanma durumu ve s�relerini tutan diziler
int taskCompletionStatus[NUM_TASKS], taskDuration[NUM_TASKS];
unsigned long long taskStartTime[NUM_TASKS], taskEndTime[NUM_TASKS];
volatile int terminateExecution = 0;
volatile int taskFinished[NUM_TASKS] = {0};

// Konsol imlecini belirtilen (x, y) koordinatlar�na ta��yan fonksiyon
void setCursorPosition(int x, int y) {
    COORD cursorPosition;
    cursorPosition.X = x;
    cursorPosition.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), cursorPosition);
}

// Konsol imlecini gizleyen fonksiyon
void hideConsoleCursor() {
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    cursorInfo.dwSize = 100;
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(consoleHandle, &cursorInfo);
}

// Konsol yaz� rengini ayarlayan fonksiyon
void setConsoleTextColor(int colorCode) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, colorCode);
}

// Sistemdeki ge�erli zaman� mikro saniye cinsinden d�nd�ren fonksiyon
unsigned long long getSystemCurrentTime() {
    FILETIME fileTime;
    GetSystemTimeAsFileTime(&fileTime);
    ULARGE_INTEGER systemTime;
    systemTime.LowPart = fileTime.dwLowDateTime;
    systemTime.HighPart = fileTime.dwHighDateTime;
    return systemTime.QuadPart / 10; // Mikro saniye cinsinden
}

// Rastgele bir gecikme s�resi �reten fonksiyon
int generateRandomDelayTime(unsigned long int *seed) {
    *seed = (*seed * 1103515245 + 12345);
    return (*seed % 400) + 600; // 10 ile 50 aras�nda rastgele bir gecikme
}

// Her bir g�revin belirtilen s�reyi tamamlamas�n� sim�le eden fonksiyon
void executeTask(int taskIndex, int iterationCount) {
    Sleep(taskDuration[taskIndex]);
    unsigned long int seed = getSystemCurrentTime() + taskIndex + iterationCount + taskDuration[taskIndex];
    // Her on bir iterasyonda g�rev s�resi rastgele olarak de�i�tiriliyor
    if (iterationCount % ((NUM_ITERATIONS / 10) + 1) == 0) {
        taskDuration[taskIndex] = generateRandomDelayTime(&seed);
    }
}

// G�revlerin ilerlemesini ekranda g�steren fonksiyon
void renderProgress() {
    for (int i = 0; i < NUM_TASKS; i++) {
        int progress = taskCompletionStatus[i] * PROGRESS_BAR_HEIGHT / NUM_ITERATIONS;

        // G�rev ilerlemesini her sat�r i�in �izme
        for (int j = 0; j < PROGRESS_BAR_HEIGHT; j++) {
            setCursorPosition(i * 5 + 5, PROGRESS_BAR_HEIGHT - j);
            if (j < progress) {
                setConsoleTextColor(60); // �lerlemi� k�sm� ye�il renk ile g�ster
                printf("-");
            } else {
                setConsoleTextColor(46); // Bo� k�sm� gri renk ile g�ster
                printf(" ");
            }
        }

        setConsoleTextColor(10); // Yaz� rengini beyaz yap
        setCursorPosition(i * 5 + 5, PROGRESS_BAR_HEIGHT + 1);
        printf("%02d", i + 1); // G�rev numaras�n� g�ster

        setCursorPosition(i * 5 + 5, PROGRESS_BAR_HEIGHT + 2);
        printf("%3d%%", (taskCompletionStatus[i] * 100) / NUM_ITERATIONS); // �lerleme y�zdesi
    }
}

// G�revler tamamland���nda nihai ilerlemeyi g�steren fonksiyon
void renderFinalProgress() {
    for (int i = 0; i < NUM_TASKS; i++) {
        // Her g�rev i�in ilerlemesi tamamlanm�� t�m barlar� doldurma
        for (int j = 0; j < PROGRESS_BAR_HEIGHT; j++) {
            setCursorPosition(i * 5 + 5, PROGRESS_BAR_HEIGHT - j);
            setConsoleTextColor(75); // Tamamlanan ilerlemeyi ye�il renk ile g�ster
            printf("-");
        }

        setConsoleTextColor(7); // Yaz� rengini beyaz yap
        setCursorPosition(i * 5 + 5, PROGRESS_BAR_HEIGHT + 1);
        printf("%02d", i + 1); // G�rev numaras�n� g�ster

        setCursorPosition(i * 5 + 5, PROGRESS_BAR_HEIGHT + 2);
        printf("100%%"); // G�rev tamamland���n� y�zde 100 olarak g�ster
    }
}

int main() {
    setlocale(LC_ALL, "Turkish"); // T�rk�e karakterlerin do�ru g�r�nmesini sa�la
    int i, j;
    hideConsoleCursor(); // Konsol imlecini gizle

    // G�rev ba�lang�� ayarlar�
    for (i = 0; i < NUM_TASKS; i++) {
        taskCompletionStatus[i] = 0;
        taskDuration[i] = generateRandomDelayTime((unsigned long int *)&i); // Rastgele g�rev s�releri olu�tur
    }

    system("cls"); // Konsolu temizle

    // Paralel g�rev i�leme
    #pragma omp parallel num_threads(NUM_TASKS)
    {
        int taskId = omp_get_thread_num(); // He                                                                      c vvvvvvvvvvvvvvvvvvv  r bir i� par�ac���na �zg� ID
        taskStartTime[taskId] = getSystemCurrentTime(); // G�rev ba�lang�� zaman�n� al

        for (j = 0; j < NUM_ITERATIONS; j++) {
            if (terminateExecution) break; // ��kma durumu kontrol�

            executeTask(taskId, j); // G�revi �al��t�r
            taskCompletionStatus[taskId] = j + 1; // G�rev tamamlanma durumunu g�ncelle

            #pragma omp critical
            renderProgress(); // Ekranda ilerlemeyi g�ncelle

            if (taskCompletionStatus[taskId] >= NUM_ITERATIONS) {
                taskFinished[taskId] = 1;
                break; // G�rev tamamland���nda ��k
            }
        }

        taskEndTime[taskId] = getSystemCurrentTime(); // G�rev biti� zaman�n� al
    }

    renderFinalProgress(); // T�m g�revler tamamland���nda nihai ilerlemeyi g�ster

    printf("\nT�m g�revler tamamland�.\n");

    // G�rev biti� zamanlar�n� ekrana yazd�r
    printf("\nG�rev Tamamlanma Zamanlar� (milisaniye cinsinden):\n");
    for (int i = 0; i < NUM_TASKS; i++) {
        printf("G�rev %02d: Ba�lang�� Zaman� = %llu ms, Biti� Zaman� = %llu ms, S�re = %llu ms\n",
               i + 1, taskStartTime[i] / 1000, taskEndTime[i] / 1000, (taskEndTime[i] - taskStartTime[i]) / 1000);
    }

    setCursorPosition(0, PROGRESS_BAR_HEIGHT + 10); // Son mesaj i�in konum ayarla
    setConsoleTextColor(7); // Yaz� rengini beyaz yap
    printf("��kmak i�in herhangi bir tu�a bas�n...");
    getch(); // Kullan�c�n�n tu�a basmas�n� bekle

    return 0;
}  

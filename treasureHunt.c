// BİLGİ: Kodtaki yorumlar yapay zeka yardımıyla yapılmıştır. -malidev

/*
 Bu program, oyuncunun bir grid üzerinde hazineleri bulmayı ve tuzaklardan kaçınmayı 
 amaçladığı bir hazine avı oyununu temsil etmektedir. Oyuncu, 5x5'lik bir grid üzerinde
 hareket ederek 3 hazineyi toplamaya çalışır ve aynı zamanda 3 tuzaktan kaçınması gerekir.
 Kod, hazinenin yerleştirilmesi, tuzakların konumlandırılması, oyuncunun hareketleri
 ve oyunun kaydedilmesi/yüklenmesi gibi çeşitli fonksiyonlar içerir.
 Bu oyunda temel olarak grid gösterimi, hazine ve tuzak yerleştirme, yön tuşlarıyla hareket,
 ipuçları, kaydetme/yükleme ve skor tablosu gibi özellikler bulunmaktadır.
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <windows.h>
#include <conio.h>
#include <signal.h>

// Oyun tahtasının boyutu ve oyun öğelerinin sayısını belirleyen sabitler
#define GRID_SIZE 5
#define NUM_TREASURES 3
#define NUM_TRAPS 3
#define MAX_FILENAME_LENGTH 100
#define MAX_PLAYER_NAME_LENGTH 50
#define MAX_LEADERBOARD_ENTRIES 100

// İmleci konumlandırmak için kullanılan makro
#define gotoxy(x,y) printf("\033[%d;%dH", (y), (x))

// ANSI renk kodları için enum tanımlaması
enum ANSI_Code {
    RESET, BOLD, UNDERLINE, MIDDLELINE, RED, LIGHTRED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE, GRAY, DARKGRAY, LIGHTGREEN,
    BG_RED, BG_LIGHTRED, BG_GREEN, BG_YELLOW, BG_BLUE, BG_MAGENTA, BG_CYAN, BG_WHITE, BG_GRAY, BG_LIGHTGREEN, ANSI_CODE_COUNT
};

// ANSI stil kodlarını içeren dizi
const char *style[ANSI_CODE_COUNT] = {
    "\033[0m", "\033[1m", "\033[4m", "\033[9m", "\033[31m", "\033[91m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m",
    "\033[37m", "\033[90m", "\033[30m", "\033[92m", "\033[41m", "\033[101m", "\033[42m", "\033[43m", "\033[44m", "\033[45m",
    "\033[46m", "\033[47m", "\033[100m", "\033[102m"
};

// Altın madalya rengi için özel tanımlama
#define MEDAL_GOLD "\033[1;33m"

// Oyun durumunu temsil eden yapı
typedef struct {
    char hiddenGrid[GRID_SIZE][GRID_SIZE];   // Gizli grid (hazineler ve tuzaklar)
    char visibleGrid[GRID_SIZE][GRID_SIZE];  // Görünür grid (oyuncunun gördüğü)
    int playerRow;                           // Oyuncunun satır konumu
    int playerCol;                           // Oyuncunun sütun konumu
    int treasuresCollected;                  // Toplanan hazine sayısı
    int moves;                               // Hamle sayısı
    char playerName[MAX_PLAYER_NAME_LENGTH]; // Oyuncu adı
} GameState;

/*
 Fonksiyon Bildirimleri (Prototipler)
 Her fonksiyonun ne yaptığına dair açıklamalar mevcuttur.
*/

// Oyunu başlatan fonksiyon
void initializeGame(GameState *game);

// Oyun tahtasını ekrana yazdıran fonksiyon
void displayGrid(GameState *game);

// Oyuncunun hareketini gerçekleştiren fonksiyon
bool makeMove(GameState *game, char direction);

// Oyuncunun yakınındaki hazine ve tuzakları hesaplayan fonksiyon
void calculateClues(GameState *game, int *nearbyTreasures, int *nearbyTraps);

// Oyunu kaydeden fonksiyon
void saveGame(GameState *game);

// Kaydedilmiş oyunu yükleyen fonksiyon
bool loadGame(GameState *game, const char *filename);

// Skor tablosunu güncelleyen fonksiyon
void updateLeaderboard(GameState *game);

// Skor tablosunu görüntüleyen fonksiyon
void displayLeaderboard();

// Kullanım bilgilerini gösteren fonksiyon
void displayUsage();

// Ekranı temizlemek için kullanılan fonksiyon
void clearScreen();

// İmleci gizleyen fonksiyon
void hideCursor();

// Konsol penceresinin yeniden boyutlandırılmasını engelleyen fonksiyon
void disableResize();

// Ana menüyü gösteren fonksiyon
void showMainMenu(GameState *game);

// Ctrl+C sinyalini yakalayan ve işleyen fonksiyon
void handleControlC(int signal);

// Oyuncunun hareketi öncesinde önizleme yapan fonksiyon
void previewMove(GameState *game, char direction, int *previewRow, int *previewCol);

// Önizleme ile birlikte oyun tahtasını gösteren fonksiyon
void displayGridWithPreview(GameState *game, int previewRow, int previewCol);

// Ana fonksiyon - program buradan başlar
int main(int argc, char *argv[]) {
    // CTRL+C sinyalini işleyecek olan fonksiyonu tanımlama
    signal(SIGINT, handleControlC);

    // UTF-8 karakter kodlamasını ayarlama
    SetConsoleOutputCP(CP_UTF8);
    // İmleci gizleme
    hideCursor();
    // Konsol yeniden boyutlandırmayı devre dışı bırakma
    disableResize();
    
    // Oyun durumunu tutacak değişken
    GameState game;
    bool loadedGame = false;
    bool displayLeaders = false;
    bool leaderboardOnly = false;
    char loadFilename[MAX_FILENAME_LENGTH] = "";
    
    // Eğer tek argüman "leaders" ise sadece skor tablosunu göster
    if (argc == 2 && strcmp(argv[1], "leaders") == 0) {
        displayLeaderboard();
        return 0;
    }
    
    // Minimum argüman sayısı kontrolü
    if (argc < 3) {
        displayUsage();
        return 1;
    }
    
    // İlk argüman "p" olmalı (oyuncu adını belirtmek için)
    if (strcmp(argv[1], "p") != 0) {
        displayUsage();
        return 1;
    }
    
    // Oyuncu adını kaydet
    strncpy(game.playerName, argv[2], MAX_PLAYER_NAME_LENGTH - 1);
    game.playerName[MAX_PLAYER_NAME_LENGTH - 1] = '\0';
    
    // Diğer argümanları işle
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "load") == 0 && i + 1 < argc) {
            strncpy(loadFilename, argv[i + 1], MAX_FILENAME_LENGTH - 1);
            loadFilename[MAX_FILENAME_LENGTH - 1] = '\0';
            loadedGame = true;
            i++;
        } else if (strcmp(argv[i], "leaders") == 0) {
            displayLeaders = true;
            if (argc == 4) {
                leaderboardOnly = true;
            }
        }
    }
    
    // Eğer istenirse lider tablosunu göster
    if (displayLeaders) {
        displayLeaderboard();
        if (leaderboardOnly) {
            return 0;
        }
    }
    
    // Ana menüyü göster
    showMainMenu(&game);
    
    // Eğer kaydedilmiş oyun yüklenecekse
    if (loadedGame) {
        if (!loadGame(&game, loadFilename)) {
            printf("%sStarting a new game because the saved game could not be loaded or belongs to another player.%s\n", 
                   style[YELLOW], style[RESET]);
            initializeGame(&game);
        }
    } else {
        // Yeni oyun başlat
        initializeGame(&game);
    }
    
    // Oyun değişkenleri
    int nearbyTreasures, nearbyTraps;
    bool gameOver = false;
    bool redisplay = true;
    int previewRow = -1;
    int previewCol = -1;
    bool previewing = false;
    char currentDirection = 0;
    
    // İlk ekran temizleme ve grid gösterimi
    clearScreen();
    displayGrid(&game);
    calculateClues(&game, &nearbyTreasures, &nearbyTraps);
    printf("%s%sUse arrow keys to preview, ENTER to confirm move, 'S' to save: %s", style[BOLD], style[WHITE], style[RESET]);
    
    // Ana oyun döngüsü
    while (!gameOver) {
        // Eğer yeniden gösterilmesi gerekiyorsa
        if (redisplay) {
            clearScreen();
            calculateClues(&game, &nearbyTreasures, &nearbyTraps);
            
            // Önizleme varsa önizleme ile birlikte göster
            if (previewing) {
                displayGridWithPreview(&game, previewRow, previewCol);
            } else {
                displayGrid(&game);
            }
            
            printf("%s%sUse arrow keys to preview, ENTER to confirm move, 'S' to save: %s", 
                   style[BOLD], style[WHITE], style[RESET]);
            redisplay = false;
        }
        
        // Tüm hazineler toplandıysa oyunu bitir
        if (game.treasuresCollected == NUM_TREASURES) {
            printf("\n%s%sYou won in %d moves!%s\n", style[GREEN], style[BOLD], game.moves, style[RESET]);
            updateLeaderboard(&game);
            gameOver = true;
            continue;
        }
        
        // Tuş basışı kontrolü
        if (_kbhit()) {
            int ch = _getch();
            
            // Ok tuşları için özel kod
            if (ch == 224) {
                ch = _getch();
                
                // Yön belirleme
                switch (ch) {
                    case 72: currentDirection = 'u'; break; // Yukarı ok
                    case 80: currentDirection = 'd'; break; // Aşağı ok
                    case 75: currentDirection = 'l'; break; // Sol ok
                    case 77: currentDirection = 'r'; break; // Sağ ok
                    default: currentDirection = 0;
                }
                
                // Geçerli bir yön seçildiyse önizleme yap
                if (currentDirection != 0) {
                    previewMove(&game, currentDirection, &previewRow, &previewCol);
                    previewing = (previewRow != game.playerRow || previewCol != game.playerCol);
                    redisplay = true;
                }
            }
            else {
                // S tuşuna basıldıysa oyunu kaydet
                if (ch == 's' || ch == 'S') {
                    saveGame(&game);
                    previewing = false;
                    redisplay = true;
                }
                // Enter tuşuna basıldıysa ve önizleme aktifse hamleyi gerçekleştir
                else if (ch == 13 && previewing) {
                    bool moveResult = makeMove(&game, currentDirection);
                    previewing = false;
                    redisplay = true;
                }
            }
        }
        
        // Kısa bekleme - CPU kullanımını azaltmak için
        Sleep(50);
    }
    
    // Oyun bittikten sonra çıkış için tuş bekle
    printf("\nPress any key to exit...");
    _getch();
    return 0;
}

// İmleci gizlemek için fonksiyon
void hideCursor() {
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(consoleHandle, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(consoleHandle, &cursorInfo);
}

// Konsol penceresinin yeniden boyutlandırılmasını engelleyen fonksiyon
void disableResize() {
    HWND consoleWindow = GetConsoleWindow();
    LONG style = GetWindowLong(consoleWindow, GWL_STYLE);
    style &= ~WS_SIZEBOX & ~WS_MAXIMIZEBOX;
    SetWindowLong(consoleWindow, GWL_STYLE, style);
    SetWindowPos(consoleWindow, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
}

// Oyunu başlatan fonksiyon - grid'i oluşturur ve hazineleri/tuzakları yerleştirir
void initializeGame(GameState *game) {
    // Oyuncuyu başlangıç konumuna yerleştir
    game->playerRow = 0;
    game->playerCol = 0;
    game->treasuresCollected = 0;
    game->moves = 0;
    
    // Grid'i başlangıç değerleriyle doldur
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            game->hiddenGrid[i][j] = ' ';
            game->visibleGrid[i][j] = '?';
        }
    }
    
    // Oyuncuyu görünür grid'de göster
    game->visibleGrid[0][0] = 'P';
    
    // Rastgele sayı üretici için seed
    srand(time(NULL));
    
    // Hazineleri rastgele yerleştir
    int treasuresPlaced = 0;
    while (treasuresPlaced < NUM_TREASURES) {
        int row = rand() % GRID_SIZE;
        int col = rand() % GRID_SIZE;
        
        // Oyuncunun başlangıç pozisyonuna hazine koyma ve aynı yere birden fazla hazine koymama kontrolü
        if ((row != 0 || col != 0) && game->hiddenGrid[row][col] == ' ') {
            game->hiddenGrid[row][col] = 'T';
            treasuresPlaced++;
        }
    }
    
    // Tuzakları rastgele yerleştir
    int trapsPlaced = 0;
    while (trapsPlaced < NUM_TRAPS) {
        int row = rand() % GRID_SIZE;
        int col = rand() % GRID_SIZE;
        
        // Oyuncunun başlangıç pozisyonuna tuzak koymama ve aynı yere birden fazla öğe koymama kontrolü
        if ((row != 0 || col != 0) && game->hiddenGrid[row][col] == ' ') {
            game->hiddenGrid[row][col] = 'X';
            trapsPlaced++;
        }
    }
    
}

// Oyun tahtasını ekrana yazdıran fonksiyon
void displayGrid(GameState *game) {
    int nearbyTreasures = 0;
    int nearbyTraps = 0;
    
    // Oyuncunun etrafındaki ipuçlarını hesapla
    calculateClues(game, &nearbyTreasures, &nearbyTraps);
    
    // Başlık çizgisi
    printf("\n%s%s%s╔═══════════════════════════════════════╗%s\n", 
           style[BOLD], style[YELLOW], style[BG_BLUE], style[RESET]);
    printf("%s%s%s║             TREASURE HUNT%s             ║%s\n", 
           style[BOLD], style[YELLOW], style[BG_BLUE], style[YELLOW], style[RESET]);
    printf("%s%s%s╚═══════════════════════════════════════╝%s\n\n", 
           style[BOLD], style[YELLOW], style[BG_BLUE], style[RESET]);
    
    // Her bir grid hücresi için
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            // Hücre içeriğine göre stil ve renk belirle
            switch (game->visibleGrid[i][j]) {
                case '?':
                    printf("%s%s ? %s", style[BG_BLUE], style[WHITE], style[RESET]);
                    break;
                case 'P':
                    printf("%s%s P %s", style[BG_GREEN], style[WHITE], style[RESET]);
                    break;
                case '=':
                    printf("%s%s = %s", style[BG_GRAY], style[WHITE], style[RESET]);
                    break;
                case 'T':
                    printf("%s%s T %s", style[BG_YELLOW], style[WHITE], style[RESET]);
                    break;
                case 'X':
                    printf("%s%s X %s", style[BG_RED], style[WHITE], style[RESET]);
                    break;
                default:
                    printf("%s%s %c %s", style[BG_WHITE], style[GRAY], game->visibleGrid[i][j], style[RESET]);
            }
        }
        
        // Sağ tarafta oyun bilgilerini göster
        switch (i) {
            case 0:
                printf("     %s%s * Collected: %d/3%s ", style[BOLD], style[GREEN], game->treasuresCollected, style[RESET]);
                printf("%s%s # Moves: %d%s", style[BOLD], style[CYAN], game->moves, style[RESET]);
                break;
            case 2:
                if (nearbyTreasures > 0) {
                    printf("     %s%s ! Near Treasures: %d%s", style[BOLD], style[YELLOW], nearbyTreasures, style[RESET]);
                }
                break;
            case 3:
                if (nearbyTraps > 0) {
                    printf("     %s%s x Near Traps: %d%s", style[BOLD], style[RED], nearbyTraps, style[RESET]);
                }
                break;
        }
        
        printf("\n");
    }
    printf("\n");
}

// Oyuncunun hareketini gerçekleştiren fonksiyon
bool makeMove(GameState *game, char direction) {
    int newRow = game->playerRow;
    int newCol = game->playerCol;
    
    // Verilen yöne göre yeni konumu hesapla
    switch (direction) {
        case 'u': newRow--; break; // Yukarı
        case 'd': newRow++; break; // Aşağı
        case 'l': newCol--; break; // Sol
        case 'r': newCol++; break; // Sağ
        default: return false;     // Geçersiz yön
    }
    
    // Grid sınırlarını aşma kontrolü
    if (newRow < 0 || newRow >= GRID_SIZE || newCol < 0 || newCol >= GRID_SIZE) {
        return false;
    }
    
    // Eğer oyuncu bir hazine üzerindeyse, hücreyi hazine olarak işaretle
    bool wasOnTreasure = (game->visibleGrid[game->playerRow][game->playerCol] == 'T');
    
    if (wasOnTreasure) {
        game->visibleGrid[game->playerRow][game->playerCol] = 'T';
    } else {
        game->visibleGrid[game->playerRow][game->playerCol] = '=';
    }
    
    // Oyuncuyu yeni konuma taşı
    game->playerRow = newRow;
    game->playerCol = newCol;
    
    // Hamle sayısını artır
    game->moves++;
    
    // Yeni konum bir hazine ise
    if (game->hiddenGrid[newRow][newCol] == 'T') {
        game->treasuresCollected++;
        game->hiddenGrid[newRow][newCol] = ' ';
        game->visibleGrid[newRow][newCol] = 'T';
        
        // Hazine bulma bildirimi göster
        clearScreen();
        displayGrid(game);
        gotoxy(0, GRID_SIZE + 8);
        printf("%s%s%s YOU FOUND A TREASURE! %s\n", style[BG_YELLOW], style[BOLD], style[YELLOW], style[RESET]);
        Sleep(1000);
        
        game->visibleGrid[newRow][newCol] = 'P';
    } 
    // Yeni konum bir tuzak ise
    else if (game->hiddenGrid[newRow][newCol] == 'X') {
        game->visibleGrid[newRow][newCol] = 'X';
        
        // Tuzağa düşme bildirimi göster
        clearScreen();
        displayGrid(game);
        
        gotoxy(0, GRID_SIZE + 8);
        printf("%s%s%s OH NO! YOU HIT A TRAP! %s\n", style[BG_RED], style[BOLD], style[WHITE], style[RESET]);
        printf("%s%sGame Over after %d moves.%s\n", style[RED], style[BOLD], game->moves, style[RESET]);
        Sleep(2000);
        printf("\nPress any key to exit...");
        _getch();
        exit(0);
    } 
    // Boş bir alan ise
    else {
        game->visibleGrid[newRow][newCol] = 'P';
    }
    
    return true;
}

// Oyuncunun yakınındaki hazine ve tuzakları hesaplayan fonksiyon
void calculateClues(GameState *game, int *nearbyTreasures, int *nearbyTraps) {
    *nearbyTreasures = 0;
    *nearbyTraps = 0;
    
    // Dört yön için değişim miktarları (yukarı, aşağı, sol, sağ)
    const int dx[] = {0, 0, -1, 1};
    const int dy[] = {-1, 1, 0, 0};
    
    // Her yönü kontrol et
    for (int i = 0; i < 4; i++) {
        int row = game->playerRow + dy[i];
        int col = game->playerCol + dx[i];
        
        // Grid sınırlarını aşma kontrolü
        if (row < 0 || row >= GRID_SIZE || col < 0 || col >= GRID_SIZE) {
            continue;
        }
        
        // Yakındaki hazine ve tuzakları say
        if (game->hiddenGrid[row][col] == 'T') {
            (*nearbyTreasures)++;
        } else if (game->hiddenGrid[row][col] == 'X') {
            (*nearbyTraps)++;
        }
    }
}

// Oyunu kaydeden fonksiyon
void saveGame(GameState *game) {
    char filename[MAX_FILENAME_LENGTH];
    char exitChoice;
    
    clearScreen();
    
    // Kaydetme menü başlığı
    printf("\n%s%s%s╔═════════════════════════════════╗%s\n", 
           style[BOLD], style[WHITE], style[BG_BLUE], style[RESET]);
    printf("%s%s%s║            SAVE GAME            ║%s\n", 
           style[BOLD], style[WHITE], style[BG_BLUE], style[RESET]);
    printf("%s%s%s╚═════════════════════════════════╝%s\n", 
           style[BOLD], style[WHITE], style[BG_BLUE], style[RESET]);
    
    // Dosya adını al
    printf("\n%sEnter filename to save (without extension): %s", style[CYAN], style[RESET]);
    scanf("%s", filename);
    
    // Dosyayı aç
    FILE *file = fopen(filename, "wb");
    if (!file) {
        printf("%s%sError opening file for saving.%s\n", style[BOLD], style[RED], style[RESET]);
        return;
    }
    
    // Oyun durumunu dosyaya yaz
    fwrite(game, sizeof(GameState), 1, file);
    fclose(file);
    
    // Başarılı kaydetme mesajı
    printf("\n%s%sGame saved as '%s'.%s\n", style[BOLD], style[GREEN], filename, style[RESET]);
    
    // Oyundan çıkma seçeneği
    printf("\n%s%sDo you want to exit the game? (y/n): %s", style[BOLD], style[YELLOW], style[RESET]);
    scanf(" %c", &exitChoice);
    
    if (exitChoice == 'y' || exitChoice == 'Y') {
        clearScreen();
        printf("\n%s%s%s╔═══════════════════════════════════════╗%s\n", 
               style[BOLD], style[GREEN], style[BG_WHITE], style[RESET]);
        printf("%s%s%s║           GAME SAVED & CLOSED         ║%s\n", 
               style[BOLD], style[GREEN], style[BG_WHITE], style[RESET]);
        printf("%s%s%s╚═══════════════════════════════════════╝%s\n\n", 
               style[BOLD], style[GREEN], style[BG_WHITE], style[RESET]);
        
        printf("%s%sOyun kapatılıyor...%s\n", style[BOLD], style[GREEN], style[RESET]);
        Sleep(1500);
        exit(0);
    }
}

// Kaydedilmiş oyunu yükleyen fonksiyon
bool loadGame(GameState *game, const char *filename) {
    // Dosyayı aç
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("%s%sError: Could not open the file '%s'.%s\n", 
               style[BOLD], style[RED], filename, style[RESET]);
        printf("\n%s%sPress any key to exit...%s\n", style[BOLD], style[YELLOW], style[RESET]);
        _getch();
        exit(1);
    }
    
    GameState loadedGame;
    
    // Dosyadan oyun durumunu oku
    if (fread(&loadedGame, sizeof(GameState), 1, file) != 1) {
        printf("%s%sError reading the save file.%s\n", style[BOLD], style[RED], style[RESET]);
        fclose(file);
        printf("\n%s%sPress any key to exit...%s\n", style[BOLD], style[YELLOW], style[RESET]);
        _getch();
        exit(1);
    }
    
    fclose(file);
    
    // Oyuncu adının eşleşip eşleşmediğini kontrol et
    if (strcmp(loadedGame.playerName, game->playerName) != 0) {
        printf("%s%sThis saved game belongs to player '%s', not to '%s'.%s\n", 
               style[BOLD], style[RED], loadedGame.playerName, game->playerName, style[RESET]);
        printf("\n%s%sPress any key to exit...%s\n", style[BOLD], style[YELLOW], style[RESET]);
        _getch();
        exit(1);
    }
    
    // Yüklenen oyun durumunu mevcut oyun durumuna kopyala
    *game = loadedGame;

    // Başarılı yükleme mesajı
    printf("\n%s%sGame '%s' loaded successfully for player '%s'.%s\n", 
           style[BOLD], style[GREEN], filename, game->playerName, style[RESET]);
    Sleep(1500);
    return true;
}

// Skor tablosunu güncelleyen fonksiyon
void updateLeaderboard(GameState *game) {
    // Skor tablosu dosyasını aç (ekleme modunda)
    FILE *file = fopen("leaderboard.txt", "a+");
    if (!file) {
        printf("%s%sError opening leaderboard file.%s\n", style[BOLD], style[RED], style[RESET]);
        return;
    }
    
    // Oyuncu adı ve hamle sayısını dosyaya yaz
    fprintf(file, "%s %d\n", game->playerName, game->moves);
    fclose(file);
    
    // Bildirim göster
    printf("%s%sYour score has been added to the leaderboard!%s\n", style[BOLD], style[GREEN], style[RESET]);
    Sleep(1000);
}

// Skor tablosunu görüntüleyen fonksiyon
void displayLeaderboard() {
    // Skor tablosu dosyasını aç
    FILE *file = fopen("leaderboard.txt", "r");
    if (!file) {
        printf("%s%sLeaderboard is empty or cannot be opened.%s\n", style[BOLD], style[RED], style[RESET]);
        Sleep(1500);
        return;
    }
    
    // Skor tablosu girdisini temsil eden yapı
    typedef struct {
        char name[MAX_PLAYER_NAME_LENGTH];
        int moves;
    } LeaderboardEntry;
    
    LeaderboardEntry entries[MAX_LEADERBOARD_ENTRIES];
    int entryCount = 0;
    
    // Dosyadan tüm skor girişlerini oku
    while (fscanf(file, "%s %d", entries[entryCount].name, &entries[entryCount].moves) == 2 &&
           entryCount < MAX_LEADERBOARD_ENTRIES) {
        entryCount++;
    }
    
    fclose(file);
    
    // Skorları hamle sayısına göre artan şekilde sırala (kabarcık sıralama)
    for (int i = 0; i < entryCount - 1; i++) {
        for (int j = 0; j < entryCount - i - 1; j++) {
            if (entries[j].moves > entries[j + 1].moves) {
                LeaderboardEntry temp = entries[j];
                entries[j] = entries[j + 1];
                entries[j + 1] = temp;
            }
        }
    }
    
    // Skor tablosunu ekrana yazdır
    clearScreen();
    printf("\n%s%s%s╔════════════════════════════════════════════╗%s\n", 
           style[BOLD], style[YELLOW], style[BG_BLUE], style[RESET]);
    printf("%s%s%s║          TREASURE HUNT LEADERBOARD         ║%s\n", 
           style[BOLD], style[YELLOW], style[BG_BLUE], style[RESET]);
    printf("%s%s%s╚════════════════════════════════════════════╝%s\n\n", 
           style[BOLD], style[YELLOW], style[BG_BLUE], style[RESET]);
    
    // Tablo başlığı
    printf("%s%s%-4s  %-20s  %-6s%s\n", style[BOLD], style[WHITE],
           "Rank", "Player", "Moves", style[RESET]);
    printf("%s%-4s  %-20s  %-6s%s\n", style[GRAY],
           "----", "--------------------", "------", style[RESET]);
    
    // En iyi 10 skoru göster
    for (int i = 0; i < entryCount && i < 10; i++) {
        const char* rankColor;
        const char* medalSymbol;
        
        // En iyi skor için altın madalya
        if (i == 0) {
            rankColor = MEDAL_GOLD;
            medalSymbol = "🏆";
        } else {
            rankColor = style[WHITE];
            medalSymbol = "  ";
        }
        
        printf("%s%s%s%-2d%s  ", style[BOLD], rankColor, medalSymbol, i + 1, style[RESET]);
        
        // İlk 3 skor için özel renklendirme
        if (i < 3) {
            printf("%s%-20s  %-6d%s\n", rankColor, entries[i].name, entries[i].moves, style[RESET]);
        } else {
            printf("%-20s  %-6d\n", entries[i].name, entries[i].moves);
        }
    }
    
    // Tuş bekleme mesajı
    printf("\n%sPress any key to %s%s", style[CYAN], 
           entryCount > 0 ? "continue..." : "start a new game...", 
           style[RESET]);
    _getch();
    clearScreen();
}

// Kullanım bilgilerini gösteren fonksiyon
void displayUsage() {
    printf("\n%s%s%s╔════════════════════════════════════════════════════════════╗%s\n", 
           style[BOLD], style[WHITE], style[BG_BLUE], style[RESET]);
    printf("%s%s%s║                 Treasure Hunt Game - Usage                 ║%s\n", 
           style[BOLD], style[WHITE], style[BG_BLUE], style[RESET]);
    printf("%s%s%s╚════════════════════════════════════════════════════════════╝%s\n\n", 
           style[BOLD], style[WHITE], style[BG_BLUE], style[RESET]);
    
    // Skor tablosu görüntüleme komutları
    printf("%s%sView Leaderboard:%s\n", style[BOLD], style[YELLOW], style[RESET]);
    printf("  %sleaders%s               : Display the leaderboard only\n\n", style[GREEN], style[RESET]);
    
    // Oyun başlatma komutları
    printf("%s%sStart Game (Required):%s\n", style[BOLD], style[YELLOW], style[RESET]);
    printf("  %sp <player_name>%s       : Specify the player name\n\n", style[GREEN], style[RESET]);
    
    // İsteğe bağlı komutlar
    printf("%s%sOptional Game Arguments:%s\n", style[BOLD], style[YELLOW], style[RESET]);
    printf("  %sload <filename>%s       : Load a saved game\n", style[GREEN], style[RESET]);
    printf("  %sleaders%s               : Show leaderboard before starting\n\n", style[GREEN], style[RESET]);
    
    // Komut örnekleri
    printf("%s%sExamples:%s\n", style[BOLD], style[YELLOW], style[RESET]);
    printf("  %s./treasureHunt leaders%s                      : Just view the leaderboard\n", style[CYAN], style[RESET]);
    printf("  %s./treasureHunt p malidev%s                    : Start new game as player 'malidev'\n", style[CYAN], style[RESET]);
    printf("  %s./treasureHunt p malidev load myGame%s        : Load 'myGame' for player 'malidev'\n", style[CYAN], style[RESET]);
    printf("  %s./treasureHunt p malidev leaders%s            : Show leaderboard before starting as 'malidev'\n", style[CYAN], style[RESET]);
    printf("  %s./treasureHunt p malidev load myGame leaders%s: Show leaderboard then load 'myGame'\n", style[CYAN], style[RESET]);
}

// Ekranı temizlemek için kullanılan fonksiyon
void clearScreen() {
    gotoxy(0, 0);
    system("cls");
}

// Ana menüyü gösteren fonksiyon
void showMainMenu(GameState *game) {
    clearScreen();
    printf("\n%s%s", style[BOLD], style[YELLOW]);
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║ %s%s                    TREASURE HUNT ADVENTURE                   %s%s%s ║\n",
           style[BG_BLUE], style[WHITE], style[RESET], style[BOLD], style[YELLOW]);
    printf("║                                                                ║\n");
    printf("║    %s%s   Find all treasures while avoiding the hidden traps!   %s%s%s   ║\n", 
           style[RESET], style[WHITE], style[RESET], style[BOLD], style[YELLOW]);
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    
    // Oyun sembollerinin açıklamaları
    printf("                    %s┌─────────────────────────┐%s\n", 
           style[WHITE], style[YELLOW]);
    printf("                    %s│   %sT : Treasure%s          │\n", 
           style[WHITE], style[YELLOW], style[RESET]);
    printf("                    %s│   %sX : Trap%s              │\n", 
           style[WHITE], style[RED], style[RESET]);
    printf("                    %s│   %sP : Player%s            │\n", 
           style[WHITE], style[LIGHTGREEN], style[RESET]);
    printf("                    %s│   %s? : Unexplored%s        │\n", 
           style[WHITE], style[BLUE], style[RESET]);
    printf("                    %s│   %s= : Explored%s          │\n", 
           style[WHITE], style[GRAY], style[RESET]);
    printf("                    %s└─────────────────────────┘\n", 
           style[WHITE]);
    
    // Başlamak için tuş bekleme
    printf("%s\nPress any key to start as player %s%s%s...", style[RESET],style[GREEN], game->playerName, style[RESET]);
    _getch();
    clearScreen();
}

// Ctrl+C sinyalini yakalayan ve işleyen fonksiyon
void handleControlC(int signal) {
    clearScreen();
    printf("\n%s%s%s╔═══════════════════════════════════════╗%s\n", 
           style[BOLD], style[RED], style[BG_WHITE], style[RESET]);
    printf("%s%s%s║           GAME BEING CLOSED!          ║%s\n", 
           style[BOLD], style[RED], style[BG_WHITE], style[RESET]);
    printf("%s%s%s╚═══════════════════════════════════════╝%s\n\n", 
           style[BOLD], style[RED], style[BG_WHITE], style[RESET]);

    printf("%s%sOyun kapatılıyor...%s\n", style[BOLD], style[RED], style[RESET]);
    Sleep(2000);
    clearScreen();
    exit(0);
}

// Oyuncunun hareketi öncesinde önizleme yapan fonksiyon
void previewMove(GameState *game, char direction, int *previewRow, int *previewCol) {
    *previewRow = game->playerRow;
    *previewCol = game->playerCol;
    
    // Verilen yöne göre önizleme konumunu hesapla
    switch (direction) {
        case 'u': (*previewRow)--; break; // Yukarı
        case 'd': (*previewRow)++; break; // Aşağı
        case 'l': (*previewCol)--; break; // Sol
        case 'r': (*previewCol)++; break; // Sağ
    }
    
    // Grid sınırlarını aşma kontrolü
    if (*previewRow < 0 || *previewRow >= GRID_SIZE || *previewCol < 0 || *previewCol >= GRID_SIZE) {
        *previewRow = game->playerRow;
        *previewCol = game->playerCol;
    }
}

// Önizleme ile birlikte oyun tahtasını gösteren fonksiyon
void displayGridWithPreview(GameState *game, int previewRow, int previewCol) {
    int nearbyTreasures = 0;
    int nearbyTraps = 0;
    
    // Oyuncunun etrafındaki ipuçlarını hesapla
    calculateClues(game, &nearbyTreasures, &nearbyTraps);
    
    // Başlık çizgisi
    printf("\n%s%s%s╔═══════════════════════════════════════╗%s\n", 
           style[BOLD], style[YELLOW], style[BG_BLUE], style[RESET]);
    printf("%s%s%s║             TREASURE HUNT%s             ║%s\n", 
           style[BOLD], style[YELLOW], style[BG_BLUE], style[YELLOW], style[RESET]);
    printf("%s%s%s╚═══════════════════════════════════════╝%s\n\n", 
           style[BOLD], style[YELLOW], style[BG_BLUE], style[RESET]);
    
    // Önizleme için ok yönünü belirle
    char previewArrow = ' ';
    if (previewRow < game->playerRow) previewArrow = '^';
    else if (previewRow > game->playerRow) previewArrow = 'v';
    else if (previewCol < game->playerCol) previewArrow = '<';
    else if (previewCol > game->playerCol) previewArrow = '>';
    
    // Her bir grid hücresi için
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            // Eğer bu hücre önizleme konumu ise ve oyuncunun mevcut konumundan farklıysa
            if (i == previewRow && j == previewCol && (previewRow != game->playerRow || previewCol != game->playerCol)) {
                printf("%s%s %c %s", style[BG_MAGENTA], style[WHITE], previewArrow, style[RESET]);
            } else {
                // Normal grid gösterimi
                switch (game->visibleGrid[i][j]) {
                    case '?':
                        printf("%s%s ? %s", style[BG_BLUE], style[WHITE], style[RESET]);
                        break;
                    case 'P':
                        printf("%s%s P %s", style[BG_GREEN], style[WHITE], style[RESET]);
                        break;
                    case '=':
                        printf("%s%s = %s", style[BG_GRAY], style[WHITE], style[RESET]);
                        break;
                    case 'T':
                        printf("%s%s T %s", style[BG_YELLOW], style[WHITE], style[RESET]);
                        break;
                    case 'X':
                        printf("%s%s X %s", style[BG_RED], style[WHITE], style[RESET]);
                        break;
                    default:
                        printf("%s%s %c %s", style[BG_WHITE], style[GRAY], game->visibleGrid[i][j], style[RESET]);
                }
            }
        }
        
        // Sağ tarafta oyun bilgilerini göster
        switch (i) {
            case 0:
                printf("     %s%s * Collected: %d/3%s ", style[BOLD], style[GREEN], game->treasuresCollected, style[RESET]);
                printf("%s%s # Moves: %d%s", style[BOLD], style[CYAN], game->moves, style[RESET]);
                break;
            case 2:
                if (nearbyTreasures > 0) {
                    printf("     %s%s ! Near Treasures: %d%s", style[BOLD], style[YELLOW], nearbyTreasures, style[RESET]);
                }
                break;
            case 3:
                if (nearbyTraps > 0) {
                    printf("     %s%s x Near Traps: %d%s", style[BOLD], style[RED], nearbyTraps, style[RESET]);
                }
                break;
        }
        
        printf("\n");
    }
    
    // Eğer önizleme aktifse, bir hareket onaylama mesajı göster
    if (previewRow != game->playerRow || previewCol != game->playerCol) {
        printf("\n%s%sPress ENTER to move to this location%s", style[BOLD], style[MAGENTA], style[RESET]);
    }
    
    printf("\n");
}
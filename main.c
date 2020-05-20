#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
// Günlük maximum musteri sayisi
#define MAX_MUSTERI 25                    

// Fonksiyon prototiplerini tanimliyoruz
void *musteri(void *num);
void *berber(void *);
void randTrasTipi();
void randMesafe();

//Global degiskenleri tanimliyoruz
static int toplamUcret = 0;
static int fiyatTarife = 0;
static int trastakiMusteriNo = 0;
static int mesafe = 0;
static char trasTipi[50] = "";

// Semaphorlari tanimliyoruz

// Bu semafor içeride kaç kisinin bekleyebilecegini belirler
sem_t beklemeOdasi;

// Bu semafor berber koltuguna sirayla oturabilmeyi saglar
//Mutual exclusion
sem_t berberKoltugu;

// Cekyat semaforu, bir müsteri gelene kadar berberin yatmasi-uyumasi içindir
sem_t cekYat;

// Trasi bekle semaforu, berber isini bitirene kadar, müsterinin beklemesini saglar
sem_t trasiBekle;

// Tüm müsterilerin trasi tamamlanana kadar 0, sonra 1 degerini alir
int tumuTamamlandi = 0;

int main() {
	pthread_t btid;
	pthread_t tid[MAX_MUSTERI];
	long RandSeed;
	int i, musteriSayisi, koltukSayisi;
	int Numarator[MAX_MUSTERI];

	printf("Musteri sayisini giriniz : "); scanf("%d", &musteriSayisi);		//Müsteri sayisini aliyoruz, en fazla 25 olarak tanimladik
	printf("Koltuk sayisini giriniz : "); scanf("%d", &koltukSayisi);		//Bekleme koltugu sayisini aliyoruz

	//Maksimum müsteri kontrolü yapiyoruz
	if (musteriSayisi > MAX_MUSTERI)
	{
		printf("Girebileceginiz en yuksek musteri sayisi %d'dir.\n", MAX_MUSTERI);
		exit(-1);
	}
	printf("*****************************************************\n");



	// Numaratör dizisini olusturuyoruz.
	for (i = 0; i < MAX_MUSTERI; i++) {
		Numarator[i] = i;
	}

	// Semaforlar, degerleri ile init ediliyor
	sem_init(&beklemeOdasi, 0, koltukSayisi);
	sem_init(&berberKoltugu, 0, 1);
	sem_init(&cekYat, 0, 0);
	sem_init(&trasiBekle, 0, 0);

	//Berber threadini olusturuyoruz
	pthread_create(&btid, NULL, berber, NULL);

	//Müsteri threadlerini olusturuyoruz
	for (i = 0; i < musteriSayisi; i++) {
		pthread_create(&tid[i], NULL, musteri, (void *)&Numarator[i]);
		sleep(1);
	}

	//Müsteri threadleri birbirlerini beklesin diye, onlari join yapiyoruz
	for (i = 0; i < musteriSayisi; i++) {
		pthread_join(tid[i], NULL);
		sleep(1);
	}

	//Bütün müsteriler bittiginde, isareti 0'dan 1'e aliyoruz
	tumuTamamlandi = 1;
	sem_post(&cekYat); // Berberi eve gitmesi için uyandiriyoruz
	pthread_join(btid, NULL);
}

//Müsteri fonksiyonu evden çikma ile baslar, dükkandan çikma ile biter
void *musteri(void *numara) {
	int num = *(int *)numara;

	//Müsteri evden çikiyor
	printf("%d numarali musteri berbere gitmek uzere evden cikiyor.\n", num);
	//Random bir mesafe ve bekleme süresi tanimliyoruz
	randMesafe();
	//Müsteri dükkana vardi, ama daha içeri girmedi. Geldigi mesafeyi gösteriyoruz
	printf("%d numarali musteri berber dukkanina vardi. Geldigi yol %d KM\n", num, mesafe);

	//Müsteri bekleme odasinda bos koltuk olmasini bekliyor ve yer olunca içeri giriyor
	sem_wait(&beklemeOdasi);
	printf("%d numarali musteri dukkana giriyor.\n", num);

	// Müsteri içeride sirasinin gelmesini bekliyor
	sem_wait(&berberKoltugu);




	// Berber koltugunun bosaldigi için kalkan müsteri, bekleme koltugunun bosaldigini bildiriyor
	sem_post(&beklemeOdasi);

	//Sirasi gelen müsteri berberi uyandiriyor
	printf("%d numarali musteri berberi uyandiriyor.\n", num);
	trastakiMusteriNo = num;
	sem_post(&cekYat);		//Çekyatin bosaldigini bildiriyoruz

	// Trasin bitmesini bekliyor
	sem_wait(&trasiBekle);

	// Tras bitti, koltugun bosaldigini bildiriyoruz
	sem_post(&berberKoltugu);
	printf("%d numarali musteri berber dukkanindan cikiyor.\n", num);
}


//Berber fonksiyonu
void *berber(void *junk) {
	//isi tamamlanmayan müsteri oldugu sürece islem devam eder
	while (!tumuTamamlandi) {

		//Firsat varken uyuyor
		printf("Berber uyuyor\n");
		sem_wait(&cekYat);


		if (!tumuTamamlandi) {

			// Rastgele bir tras tipi ve bekleme süresi belirliyoruz
			randTrasTipi();
			//Berber tras yapiyor ve trasi bitiriyor
			printf("Berber %d numarali musteriyi %s  ediyor\n", trastakiMusteriNo, trasTipi);
			printf("Berber %d numarali musterinin trasini bitirdi ve %d TL aldi.\n", trastakiMusteriNo, fiyatTarife);

			// Trasi bekle semaforuna trasin bittigini bildir
			sem_post(&trasiBekle);
		}
		else {
			//Berber tüm müsterilerinin isini bitirdi ve eve gidiyor
			printf("*****************************************************\n");
			printf("Berber, dukkani kapatti ve eve gidiyor. %d TL ciro yapti \n", toplamUcret);
			//tumuTamamlandi = 0;

			//Tekrar main fonksiyonunu çagirarak bastan baslayabiliriz
			//main();
		}
	}
}

void randTrasTipi() {
	int len;

	srand(time(NULL));	//Rastgele bir sayi beliliyoruz
	len = rand() % 3; // Sayi 0 , 1 , 2 olabilir

	//sayiya göre tras tipi ve ücret belirliyoruz
	switch (len)
	{
	case 0:
		//printf("Sac trasi\n");
		strcpy(trasTipi, "Sac trasi");
		fiyatTarife = 35;
		toplamUcret = toplamUcret + fiyatTarife;
		break;
	case 1:
		//printf("Sakal trasi\n");
		strcpy(trasTipi, "Sakal trasi");
		fiyatTarife = 20;
		toplamUcret = toplamUcret + fiyatTarife;
		break;
	case 2:
		//printf("Sac Sakal karisik\n");
		strcpy(trasTipi, "Sac Sakal karisik trasi");
		fiyatTarife = 50;
		toplamUcret = toplamUcret + fiyatTarife;
		break;
	}
	sleep(len);
}



void randMesafe() {
	int len;

	// Rastgele bir sayi belirliyoruz

	srand(time(NULL));
	len = rand() % 20;	// sayi 0-20 arasi olabilir
	sleep(len);
	mesafe = len;	//Müsteri bu yoldan gelecek


}



$set 1 #BaseDisplay

1 %s : X hatas� : %s ( %d ) opkodlar %d/%d\n  kaynak 0x%lx\n
2 %s : %d sinyali al�nd� \n
3 kapat�l�yorum\n
4 kapat�l�yorum ... ��k�nt�y� b�rak�yorum\n
5 BaseDisplay::BaseDisplay : X sunucusuna ba�lan�l�namad� .\n
6 BaseDisplay::BaseDisplay : �al��t�r�rken kapatmada ekrana ba�lan�l�namad� \n
7 BaseDisplay::eventLoop() : eylem kuyru�undaki 'k�t�' pencereyi siliyorum \n

$set 2 #Basemenu

1 Blackbox m�n�s�

$set 3 #Configmenu

1 Ayarlar
2 Fok�sleme
3 Pencere yerle�imi
4 Resim olu�turmas�
5 Ekran� i�erikli ta��
6 Tam ekranla, vallahi
7 Yeni pencereleri fok�sle
8 Masa�st�ndeki son pencereyi fok�sle
9 T�klayarak fok�sle
10 A��r fok�sle
11 Otomatikman y�kselt
12 Ak�ll� yerle�im( S�ralar )
13 Ak�ll� yerle�im( S�tunlar )
14 Cascade Placement
15 Soldan sa�a
16 Sa�dan sola
17 �stten a�a�a
18 Alttan �ste

$set 4 #Icon

1 Ikonalar
2 Isimsiz

$set 5 #Image

1 BImage::render_solid : resmi yaratamad�m\n
2 BImage::renderXImage : XImage'i yaratamad�m\n
3 BImage::renderXImage : desteklenmeyen g�r�n��( renk derinli�i )\n
4 BImage::renderPixmap : resmi yaratamad�m\n
5 BImageControl::BImageControl : ge�ersiz renk haritas� b�y�kl��� %d (%d/%d/%d) - azalt�yorum\n
6 BImageControl::BImageControl : renk haritas� ayr�lanamad�\n
7 BImageControl::BImageControl : rengi ayr�rken hata oldu : %d/%d/%d\n
8 BImageControl::~BImageControl : resim arabelle�i - %d resim temizlendi\n
9 BImageControl::renderImage : arabellek b�y�k, temizlemeye ba�l�yorum\n
10 BImageControl::getColor : renk tarama hatas� : '%s'\n
11 BImageControl::getColor : renk ay�rma hatas� : '%s'\n

$set 6 #Screen

1 BScreen::BScreen : X sunucusunu sorgularken hata oldu.\n  \
%s ekran�nda ba�ka bir pencere y�neticisi �al���yor gibi.\n
2 BScreen::BScreen : %d ekran�, 0x%lx g�r�n�m�yle , %d derinli�iyle\n
3 BScreen::LoadStyle() : '%s' yaz� tipi y�klenemedi.\n
4 BScreen::LoadStyle(): �nayarl� yaz� tipi y�klenemedi.\n
5 %s : bo� m�n� dosyas�\n
6 X komutas�
7 Yeniden ba�la
8 ��k
9 BScreen::parseMenuFile : [exec] hatas�, m�n� yaftas� ve/yada komuta belirlenmedi\n
10 BScreen::parseMenuFile : [exit] hatas�, m�n� yaftas� belirlenmedi\n
11 BScreen::parseMenuFile : [style] hatas�, m�n� yaftas� ve/yada dosya ad� belirlenmedi\n
12 BScreen::parseMenuFile: [config] hatas�, m�n� yaftas� belirlenmedi\n
13 BScreen::parseMenuFile: [include] hatas�, dosya ad� belirlenmedi\n
14 BScreen::parseMenuFile: [include] hatas�, '%s' vasat bir dosya de�il\n
15 BScreen::parseMenuFile: [submenu] hatas�, m�n� yaftas� belirlenmedi\n
16 BScreen::parseMenuFile: [restart] hatas�, m�n� yaftas� belirlenmedi\n
17 BScreen::parseMenuFile: [reconfig] hatas�, m�n� yaftas� belirlenmedi\n
18 BScreen::parseMenuFile: [stylesdir/stylesmenu] hatas�, dizin ad� belirlenmedi\n
19 BScreen::parseMenuFile: [stylesdir/stylesmenu] hatas�, '%s' bir dizin \
de�ildir\n
20 BScreen::parseMenuFile: [stylesdir/stylesmenu] hatas�, '%s' var de�il\n
21 BScreen::parseMenuFile: [workspaces] hatas�, m�n� yaftas� belirlenmedi\n
22 0: 0000 x 0: 0000
23 X: %4d x Y: %4d
24 Y: %4d x E: %4d

$set 7 #Slit

1 Slit
2 Slit y�n�
3 Slit yerle�imi

$set 8 #Toolbar

1 00:00000
2 %02d/%02d/%02d
3 %02d.%02d.%02d
4  %02d:%02d 
5 %02d:%02d %sm
6 p
7 a
8 Blackbox �ubu�u
9 Ge�erli masa�st� ismini de�i�tir
10 Blackbox �ubu�unun yerle�imi

$set 9 #Window


1 BlackboxWindow::BlackboxWindow : 0x%lx'i yarat#_yorum\n
2 BlackboxWindow::BlackboxWindow : XGetWindowAttributres ba�ar�s�z oldu\n
3 BlackboxWindow::BlackboxWindow : 0x%lx ana penceresi i�in ekran� belirleyemedim\n
4 Isimsiz
5 0x%lx i�in BlackboxWindow::mapRequestEvent()\n
6 0x%lx i�in BlackboxWindow::unmapNotifyEvent()\n
7 BlackboxWindow::unmapnotifyEvent: 0x%lx'i ana pencereyi boya\n

$set 10 #Windowmenu

1 G�nder ...
2 Topla
3 Ikonala�t�r
4 Azamile�tir
5 Al�alt
6 Y�kselt
7 Yap���k
8 �ld�r
9 Kapat

$set 11 #Workspace

1 Masa�st� %d

$set 12 #Workspacemenu

1 Masa�stleri
2 Yeni bir masa�st�
3 Son masa�st�n� sil

$set 13 #blackbox

1 Blackbox::Blackbox: y�netebilinen ekran bulunamad�, bitiriliyorum\n
2 Blackbox::process_event: 0x%lx i�in MapRequest\n

$set 14 #Common

1 Evet
2 Hay�r

3 Y�n
4 Ufki
5 Dikey

6 Her zaman �stte

7 Yerle�im
8 Sol �st
9 Sol orta 
10 Sol alt
11 �st orta
12 Alt orta
13 Sa� �st
14 Sa� orta
15 Sa� �st

$set 15 #main

1 hata : '-rc' bir arg�man bekler\n
2 hata : '-display' bir arg�man bekler\n
3 ikaz : 'DISPLAY' verisini oturtamad�m\n
4 Blackbox %s: (c) 1997 - 2000 Brad Hughes\n\n\
  -display <metin>\t\tekran� kullan.\n\
  -rc <metin>\t\t\tba�ka bir ayarlama dosyas�n� kullan.\n\
  -version\t\t\tnesil bilgisini g�sterir ve ��kar.\n\
  -help\t\t\t\tbu yard�m iletisini g�sterir ve ��kar.\n\n
5 Denetleme se�enekleri :\n\
  Bilgilendirme\t\t\t%s\n\
  T�zlama:\t\t\t%s\n\
  G�lgeleme:\t\t\t%s\n\
  Slit:\t\t\t\t%s\n\
  R8b'e g�re t�zla:\t%s\n\n

$set 16 #bsetroot

1 %s : hata : -solid, -mod yada -gradient'den birisini belirlemek zorundas�n\n
2 %s 2.0 : Tel'if hakk� (c) 1997-2000 Brad Hughes\n\n\
  -display <metin>         ekran belirlemesi\n\
  -mod <x> <y>             b�l���m i�lemi\n\
  -foreground, -fg <renk>  b�l���m �nalan�\n\
  -background, -bg <renk>  b�l���m ardalan�\n\n\
  -gradient <kaplam>       ge�i�im kaplam�\n\
  -from <renk>             ge�i�im ba�lama rengi\n\
  -to <renk>               ge�i�im biti� rengi\n\n\
  -solid <renk>            tek renk\n\n\
  -help                    bu yard�m iletisini g�ster ve ��k\n


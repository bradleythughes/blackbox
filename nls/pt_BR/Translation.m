$set 1 #BaseDisplay

1 %s:  X erro: %s(%d) opcodes %d/%d\n  recurso 0x%lx\n
2 %s: recebeido sinal %d\n
3 desligando\n
4 abortando... gerando arquivo core\n
5 BaseDisplay::BaseDisplay: conec��o ao servidor X falhou.\n
6 BaseDisplay::BaseDisplay: impossivel marcar a conex�o ao display como close-on-exec\n
7 BaseDisplay::eventLoop(): removendo m� janela da trilha de eventos\n

$set 2 #Basemenu

1 Menu Blackbox

$set 3 #Configmenu

1 Configura��es
2 Modelo de Foco
3 Posicionamento das Janelas
4 Simula��o de Cores em Imagens
5 Movimento Opaco de Janelas
6 Maximiza��o Total
7 Foco em Novas Janelas
8 Foco na Ultima Janela Da Area de Trabalho
9 Foco na Janela Quando Clicada
10 Foco na Janela Quando Apontada
11 Sobrepor Janela Quando Ativa
12 Posicionamento em Fila
13 Posicionamento em Coluna
14 Posicionamento em Cascata
15 Esquerda para Direita
16 Direita para Esquerda
17 Cima para Baixo
18 Baixo para Cima

$set 4 #Icon

1 Icones
2 Sem nome

$set 5 #Image

1 BImage::render_solid: erro criando pixmap\n
2 BImage::renderXImage: erro criando XImage\n
3 BImage::renderXImage: visual n�o suportado\n
4 BImage::renderPixmap: erro criando pixmap\n
5 BImageControl::BImageControl: mapa de cores de tamanho invalido %d (%d/%d/%d) - reduzindo\n
6 BImageControl::BImageControl: erro localizando mapa de cores\n
7 BImageControl::BImageControl: falha localizando cor %d/%d/%d\n
8 BImageControl::~BImageControl: reserva de pixmap cheia - liberando %d pixmaps\n
9 BImageControl::renderImage: reserva cheia, for�ando limpeza\n
10 BImageControl::getColor: color parse error: '%s'\n
11 BImageControl::getColor: erro localizando cor: '%s'\n

$set 6 #Screen

1 BScreen::BScreen: um erro occorreu enquanto chamando o servidor X.\n  \
existe outro window menager rodando no display. %s.\n
2 BScreen::BScreen: managing screen %d using visual 0x%lx, depth %d\n
3 BScreen::LoadStyle(): impossivel carregar fonte '%s'\n
4 BScreen::LoadStyle(): impossivel carregar fonte padr�o.\n
5 %s: arquivo de menu vazio\n
6 xterm
7 Reiniciar
8 Sair
9 BScreen::parseMenuFile: [exec] erro, n�o h� indica��o definida no menu e/ou comando definido\n
10 BScreen::parseMenuFile: [exit] erro, n�o h� indica��o definida no menu\n
11 BScreen::parseMenuFile: [style] erro, n�o h� indica��o de menu e/ou nome de arquivo difinido\n
12 BScreen::parseMenuFile: [config] erro, n�o h� indica��o definida no menu\n
13 BScreen::parseMenuFile: [include] erro, nome de arquivo n�o definido\n
14 BScreen::parseMenuFile: [include] error, '%s' n�o � um arquivo regular\n
15 BScreen::parseMenuFile: [submenu] erro, n�o h� indica��o definida no menu\n
16 BScreen::parseMenuFile: [restart] erro, n�o h� indica��o definida no menu\n
17 BScreen::parseMenuFile: [reconfig] erro, n�o h� indica��o definida no menu\n
18 BScreen::parseMenuFile: [stylesdir/stylesmenu] erro, n�o h� diretorio difinido\n
19 BScreen::parseMenuFile: [stylesdir/stylesmenu] erro, '%s' n�o � um diretorio\n
20 BScreen::parseMenuFile: [stylesdir/stylesmenu] erro, '%s' n�o existe\n
21 BScreen::parseMenuFile: [workspaces] erro, n�o h� indica��o definida no menu\n
22 0: 0000 x 0: 0000
23 X: %4d x Y: %4d
24 W: %4d x H: %4d


$set 7 #Slit

1 Slit
2 Dire��o
3 Posicionamento

$set 8 #Toolbar

1 00:00000
2 %02d/%02d/%02d
3 %02d.%02d.%02d
4  %02d:%02d 
5 %02d:%02d %sm
6 p
7 a
8 Toolbar
9 Renomear Area de Trabalho
10 Posicionamento

$set 9 #Window


1 BlackboxWindow::BlackboxWindow: criando 0x%lx\n
2 BlackboxWindow::BlackboxWindow: XGetWindowAttributres falhou\n
3 BlackboxWindow::BlackboxWindow: impossivel encontrar tela para janela 0x%lx\n
4 Sem Nome
5 BlackboxWindow::mapRequestEvent() para 0x%lx\n
6 BlackboxWindow::unmapNotifyEvent() para 0x%lx\n
7 BlackboxWindow::unmapnotifyEvent: reparent 0x%lx to root\n

$set 10 #Windowmenu

1 Enviar Para ...
2 Encolher
3 Minimizar
4 Maximizar
5 Focalizar
6 Desfocalizar
7 Sempre Visivel
8 Kill Cliente
9 Fechar

$set 11 #Workspace

1 Area de Trabalho %d

$set 12 #Workspacemenu

1 Areas de Trabalho
2 Nova Area de Trabalho
3 Remover Ultima

$set 13 #blackbox

1 Blackbox::Blackbox: n�o foram encontradas telas administraveis, abortando..\n
2 Blackbox::process_event: MapRequest para 0x%lx\n

$set 14 #Common

1 Sim
2 N�o

3 Dire��o
4 Horizontal
5 Vertical

6 Sempre Visivel

7 Posicionamento
8 No Alto a Esquerda
9 No Centro a Esquerda
10 Em Baixo a Esquerda
11 No Alto e no Centro
12 Em Baixo e no Centro
13 No Alto a Direita
14 No Meio a Direita
15 Em Baixo a Direita

$set 15 #main

1 erro: '-rc' requer um argumento\n
2 erro: '-display' requer um argumento\n
3 aviso: n�o foi possivel setar a variavel de ambiente 'DISPLAY'\n
4 Blackbox %s: (c) 1997 - 2000 Brad Hughes\n\n\
  -display <string>\t\tusar conex�o com o display.\n\
  -rc <string>\t\t\tusar arquivo alternativo de recursos.\n\
  -version\t\t\tmostrar versao e sair.\n\
  -help\t\t\t\tmostrar esta ajuda e sair.\n\n
5 Op��es em tempo de compila��o:\n\
  Informa��es extras para problemas:\t\t\t%s\n\
  Interlacing:\t\t\t%s\n\
  Forma:\t\t\t%s\n\
  Slit:\t\t\t\t%s\n\
  8bpp Simula��o ordenada de cores em imagens:\t%s\n\n

$set 16 #bsetroot

1 %s: erro: necessario especificar um dos seguintes argumentos: -solid, -mod, -gradient\n
2 %s 2.0: (c) 1997-2000 Brad Hughes\n\n\
  -display <string>        conex�o com display\n\
  -mod <x> <y>             modula pattern\n\
  -foreground, -fg <color> modula foreground color\n\
  -background, -bg <color> modula background color\n\n\
  -gradient <texture>      textura gradiente\n\
  -from <color>            cor de inicio do gradiente\n\
  -to <color>              com do fim do gradiente\n\n\
  -solid <color>           cor solida\n\n\
  -help                    mostra essa ajuda e sai\n


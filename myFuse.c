//comecamos utilizando caa item que esta na struct fuse_operations. Tudo o que esta la nao precisa ser implementado
//mas sim apenas utilizado para conseguir estruturar um sistema de arquivos propriamente

/*Todos os campos que estao na estrutura nao sao as funcoes em si, mas sim ponteiros que levam para as funcoes.
Cada campo é chamado pelo FUSE quando um evento específico acontece no sistema de arquivos.
Por exemplo: se um usuário deseja escrever em um arquivo a funcao apontada pelo campo ¨write¨ na estrutura será chamada.
Muitos campos são semelhantes aos que usamos nos terminais do linux, como por exemplo; mkdir, que cria diretorios novos, rename para alterar o
nome de um arquivo, unlink para apagar um arquivo, etc.*/

/*Para implementarmos este sistema de arquivos precisamos usar esta estrutura fuse_operations
depois precisamos definir as funcoes dessa estrutura neste código,
depois preencher a estrutura com os ponteiros das minhas funcoes implementadas*/

#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
//#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>


//a funcao ¨do_getattr¨ sera a funcao chamada quando o sistema pedir
//o SSFS (secure storage in file system) para os atributos de um arquivo especifico

/*O primeiro parametro é o caminho que o sistema pediu para o SSFS pelos atributos daquele arquivo
O outro é a estrutura stat que precisa ser preenchida com os atributos daquele arquivo
Tendo sucesso, o valor retornado precisa ser 0, caso contrario precisa ser -1
e o errno precisa ser preenchido com o codigo de erro correto*/
static int do_getattr(const char *path, struct stat *st)
{
    printf( "[getattr] Called\n" );
	printf( "\tAttributes of %s requested\n", path );
    
    //st_uid representa o propietário do arquivo em questão
    //No SSFS o proprietario de todos os arquivos e diretorios é o mesmo
    //usuario que deu ¨mount¨ no sistema de arquivos
    //entào este é o id de uma só pessoa
    st->st_uid = getuid();

    //st_gid, que representa o grupo proprietario dos arquivos/diretorios
    //será o mesmo grupo do usuario que deu mount no sistema de arquivos
    //este é o id do grupo, que por consequencia a pessoa acima acaba fazendo parte
    st->st_gid = getgid();

    //a funcao time pega o horario e data atual
    //st_atime representa a ultima data de acesso do arquivo
    st->st_atime = time(NULL);
    //st_mtime representa a data da ultima modificacao do arquivo
    st->st_mtime = time(NULL);

    //strcmp() é uma funcao que compara duas strings
    //então neste caso se o path for igual a "/" 
    //significa o arquivo em questao é o root directory do SSFS
    if(strcmp(path, "/") == 0)
    {
        //st_mode especifica especifica se o arquivo é só um arquivo comum
        //um diretorio, ou qualquer outra coisa (com __S_IFDIR -> indica se é diretório). 
        //além disso ele especifica a permissào de bits daquele arquivo

        /*definimos a permissão de bits como: apenas o proprietario do arquivo
        pode ler, escrever e executar o diretorio, os usuarios do grupo só podem ler e executar o diretorio.*/
        st->st_mode = __S_IFDIR | 0755;

        //st_nlink especifica o numero de "hardlinks"

        /*When you create a hard link the hard link is yet another reference to the same inode as the original file.
        A hard link allows a user to create two exact files without having to duplicate the data on disk. 
        However unlike creating a copy, if you modify the hard link you are in turn modifying the original file as well as they both reference the same inode.*/

        //porque 2 hardlinks: https://unix.stackexchange.com/questions/101515/why-does-a-new-directory-have-a-hard-link-count-of-2-before-anything-is-added-to/101536#101536
        st->st_nlink = 2;
    }
    //todos os arquivos que não são o root directory caem aqui
    else
    {
        //__S_IFREG é apenas para indicar que os arquivos são apenas arquivos regulares
        /*A permissao de bits é: o proprietario pode ler e escrever o arquivo
        enquanto o resto do grupo só pode apenas ler*/
        st->st_mode = __S_IFREG | 0644;
        st->st_nlink = 1;

        //especifica o tamanho do arquivo em bytes
        st->st_size = 1024;
    }

    return 0;

}

//em readdir podemos fazer a lista de arquivos/diretorios que estao disponiveis dentro de um diretorio.
//no SSFS só existe um diretorio (O root directory)

//Os parametros: path é o caminho do diretório que o sistema pediu a lista de arquivos que reside dentro dele
//buffer é onde preenchemos com nomes dos arquivos/directories que estão dentro do diretório em questão
//filler é uma função enviada pelo fuse e que podemos usar para preencher o buffer com arquivos disponiveis no path

/*filler dentro do fuse é declarado da seguinte:
typedef int (*fuse_fill_dir_t) (void *buf, const char *name,
				const struct stat *stbuf, off_t off);
                
* The first parameter is a pointer to the buffer which we want to write the entry (filename or directory name) on. 
The second parameter is the name of the current entry. 
The third and the fourth parameters will not be covered here.*/
static int do_readdir(const char* path, void* buffer, fuse_fill_dir_t filler,
off_t offset, struct fuse_file_info* fi)
{
    /*We filled the list of available entries in “path” with two entries: “.” which represents the current directory, 
    while “..” represents the parent directory. 
    It’s a known convention in Unix world.*/
    filler(buffer, ".", NULL, 0); //Diretorio atual
    filler(buffer, "..", NULL, 0); //Diretorio pai

    //Se o usuario quiser mostrar os arquivos/diretorios do root directory
    if ( strcmp( path, "/" ) == 0 )
	{
		filler( buffer, "file54", NULL, 0 );
		filler( buffer, "file349", NULL, 0 );
	}
	
	return 0;
}

//read consegue ler o conteúdo de algum arquivo específico
//Neste buffer vamos guardar o pedaço que o sistema está interessado
//size é o tamanho do pedaço 
//offset é o lugar no conteúdo do qual vamos começar a ler
//esta função precisa retornar o número de bytes que foram lidos com sucesso
//basicamente lemos letra por letra. Se size = 35 e offset = 0, então ele vai ler 
//da primeira letra do arquivo até a letra 35
/*Outro exemplo, vamos supor que size = 35 mas o offset = 40, 
então vamos pular os primeiros 41 caracteres, 
pois offset = 40, e começar a ler do caracter 42 ao 77, pois o size = 35*/
static int do_read(const char* path, char* buffer, size_t size, 
off_t offset, struct fuse_file_info* fi)
{
    char file54Text[] = "Hello World From File54!";
	char file349Text[] = "Hello World From File349!";
	char *selectedText = NULL;

    //Se for o file54, passamos o ponteiro dele para o selectedText
    if ( strcmp( path, "/file54" ) == 0 )
    {
        selectedText = file54Text;
    }
	else if ( strcmp( path, "/file349" ) == 0 )
    {
        selectedText = file349Text;
    }	
    //Se tentar ler qualquer outro arquivo, retorna erro
	else
    {
        return -1;
    }
	
    //Copiamos para o buffer o conteúdo do arquivo em questão
    //começando pelo offset até chegar no size
    //depois retornamos o número de bytes lido
    memcpy( buffer, selectedText + offset, size );
		
	return strlen( selectedText ) - offset;
}

//Agora preenchemos o fuse_operations e chamamos a função main do fuse
//que rodará o nosso sistema de arquivos

static struct fuse_operations operations = {
    .getattr	= do_getattr,
    .readdir	= do_readdir,
    .read	= do_read,
};

int main( int argc, char *argv[] )
{
	return fuse_main( argc, argv, &operations, NULL );
}

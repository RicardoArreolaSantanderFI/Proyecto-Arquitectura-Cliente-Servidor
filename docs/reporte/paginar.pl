# Lee el PDF con pdftotext y escribe paginas.txt con la pagina donde inicia cada seccion.
use strict; use warnings;
my ($pdf, $dir) = @ARGV;
my @secciones = (
    ['introduccion',   'INTRODUCCIÓN'],
    ['arquitectura',   'Arquitectura cliente-servidor'],
    ['sockets',        'Sockets TCP/IP'],
    ['dbms',           'Sistema manejador de bases de datos'],
    ['sql',            'Lenguaje SQL'],
    ['objetivos',      'OBJETIVOS'],
    ['desarrollo',     'DESARROLLO'],
    ['estructura',     'Estructura del proyecto'],
    ['protocolo',      'Constantes compartidas'],
    ['util',           'Funciones de apoyo'],
    ['servidor',       'Servidor'],
    ['cliente',        'Cliente'],
    ['tabla',          'Manejo de la tabla'],
    ['interprete',     'Intérprete de comandos SQL'],
    ['makefile',       'Makefile'],
    ['resultados',     'RESULTADOS'],
    ['conclusiones',   'CONCLUSIONES'],
    ['bibliografia',   'BIBLIOGRAFÍA Y FUENTES'],
);
my $texto = `pdftotext -enc UTF-8 "$pdf" -`;
my @paginas = split /\f/, $texto;
open(my $o, '>', "$dir/paginas.txt") or die;
my $desde = 2;                            # se omiten caratula e indice
for my $s (@secciones) {
    my ($id, $titulo) = @$s;
    my $encontrada = '';
    for my $i ($desde .. $#paginas) {     # cada seccion va despues de la anterior
        if ($paginas[$i] =~ /^\s*\Q$titulo\E\s*$/m) { $encontrada = $i + 1; $desde = $i; last; }
    }
    print $o "$id=$encontrada\n";
    print STDERR "$titulo -> $encontrada\n";
}
close $o;

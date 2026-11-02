# Genera reporte.html a partir de plantilla.html.
# {{CODIGO:ruta}} o {{CODIGO:ruta:inicio-fin}} inserta el codigo con numeros de linea.
# {{PAG:id}} inserta el numero de pagina leido de paginas.txt, si existe.
use strict; use warnings;
my ($raiz, $dir) = @ARGV;   # raiz del repo, carpeta del reporte

my %pag;
if (open(my $p, '<', "$dir/paginas.txt")) {
    while (<$p>) { chomp; my ($k, $v) = split /=/; $pag{$k} = $v if defined $v; }
    close $p;
}

open(my $t, '<:encoding(UTF-8)', "$dir/plantilla.html") or die "sin plantilla";
my $html = do { local $/; <$t> }; close $t;

sub escapar { my $s = shift; $s =~ s/&/&amp;/g; $s =~ s/</&lt;/g; $s =~ s/>/&gt;/g; return $s; }

sub codigo {
    my ($ruta, $ini, $fin) = @_;
    open(my $f, '<:encoding(UTF-8)', "$raiz/$ruta") or die "no existe $ruta";
    my @lineas = <$f>; close $f;
    $ini ||= 1; $fin ||= scalar @lineas;
    my $out = '';
    for my $i ($ini .. $fin) {
        my $l = $lineas[$i - 1]; $l = '' unless defined $l; chomp $l;
        $l =~ s/\t/    /g;
        $out .= sprintf("<span class=\"num\">%3d</span>  %s\n", $i, escapar($l));
    }
    print STDERR sprintf("%-16s %3d-%3d | %s ... %s\n", $ruta, $ini, $fin,
        substr(($lineas[$ini-1] // ''), 0, 40) =~ s/\s+$//r, substr(($lineas[$fin-1] // ''), 0, 40) =~ s/\s+$//r);
    return "<pre class=\"codigo\">$out</pre>";
}

$html =~ s/\{\{CODIGO:([^:}]+)(?::(\d+)-(\d+))?\}\}/codigo($1, $2, $3)/ge;
$html =~ s/\{\{PAG:([^}]+)\}\}/exists $pag{$1} ? $pag{$1} : ''/ge;

open(my $o, '>:encoding(UTF-8)', "$dir/reporte.html") or die; print $o $html; close $o;
print STDERR "reporte.html generado\n";

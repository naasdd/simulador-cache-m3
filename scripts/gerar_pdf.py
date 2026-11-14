#!/usr/bin/env python3
"""
Gera docs/relatorio.pdf a partir de docs/relatorio.md, no padrão formal da faculdade:
capa UNIVALI, Times New Roman 12, espaçamento 1,5, margens ABNT (3/3/2/2 cm),
número da página no rodapé à direita (a capa conta como página 1, mas não mostra o número).

Usa só a biblioteca padrão do Python + o Google Chrome em modo headless pra imprimir o PDF.

Autores: José Gabriel Santos Gomes, Matheus Pompeo Dias, Nathan Gustavo Reichert
"""
import html
import re
import subprocess
import sys
import tempfile
from pathlib import Path

PASTA_DO_PROJETO = Path(__file__).resolve().parent.parent
ARQUIVO_MARKDOWN = PASTA_DO_PROJETO / "docs" / "relatorio.md"
ARQUIVO_PDF = PASTA_DO_PROJETO / "docs" / "relatorio.pdf"
CAMINHO_DO_CHROME = "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome"

DADOS_DA_CAPA = {
    "universidade": "UNIVERSIDADE DO VALE DO ITAJAÍ",
    "curso": "Curso de Ciência da Computação",
    "alunos": ["José Gabriel Santos Gomes", "Matheus Pompeo Dias", "Nathan Gustavo Reichert"],
    "disciplina": "Organização de Computadores",
    "professor": "Thiago Felski Pereira, M.Sc.",
    "semestre": "2026/2",
    "cidade": "Itajaí (SC)",
    "ano": "2026",
}

ESTILO = """
@page {
  size: A4;
  margin: 3cm 2cm 2cm 3cm;
  @bottom-right { content: counter(page); font-family: 'Times New Roman', Times, serif; font-size: 12pt; }
}
@page :first { @bottom-right { content: none; } }

body { font-family: 'Times New Roman', Times, serif; font-size: 12pt; line-height: 1.5; color: #000; margin: 0; }

.capa { height: 24.6cm; display: flex; flex-direction: column; justify-content: space-between;
        text-align: center; break-after: page; }
.capa p { margin: 0; text-indent: 0; text-align: center; }
.capa .titulo { font-weight: bold; text-transform: uppercase; }

p { text-align: justify; text-indent: 1.25cm; margin: 0 0 12pt 0; }
h2 { font-size: 12pt; font-weight: bold; text-transform: uppercase; margin: 24pt 0 12pt 0; break-after: avoid; }
h3 { font-size: 12pt; font-weight: bold; margin: 18pt 0 12pt 0; break-after: avoid; }
h2 + p, h3 + p { break-before: avoid; }

ol, ul { margin: 0 0 12pt 0; padding-left: 1.25cm; }
li { text-align: justify; margin-bottom: 6pt; }

table { border-collapse: collapse; margin: 6pt auto 14pt auto; font-size: 10.5pt; line-height: 1.25;
        break-inside: avoid; }
th, td { border: 1px solid #000; padding: 3pt 7pt; }
th { font-weight: bold; background: #eee; }
td.direita, th.direita { text-align: right; }

pre { font-family: 'Courier New', Courier, monospace; font-size: 9.5pt; line-height: 1.25;
      border: 1px solid #999; padding: 6pt 9pt; margin: 6pt 0 14pt 0; white-space: pre-wrap; break-inside: avoid; }
code { font-family: 'Courier New', Courier, monospace; font-size: 10.5pt; }

.referencia { text-indent: 0; text-align: left; padding-left: 0; margin-bottom: 12pt; line-height: 1.0; }
"""


def formatarTextoDaLinha(texto):
    """Converte **negrito**, *itálico* e `código` em HTML (escapando o resto)."""
    texto = html.escape(texto, quote=False)
    texto = re.sub(r"`([^`]+)`", r"<code>\1</code>", texto)
    texto = re.sub(r"\*\*([^*]+)\*\*", r"<strong>\1</strong>", texto)
    texto = re.sub(r"(?<![*\w])\*([^*]+)\*(?![*\w])", r"<em>\1</em>", texto)
    return texto


def converterTabela(linhasDaTabela):
    def separarCelulas(linha):
        return [celula.strip() for celula in linha.strip().strip("|").split("|")]

    cabecalho = separarCelulas(linhasDaTabela[0])
    alinhamentos = separarCelulas(linhasDaTabela[1])
    alinhadaADireita = [alinhamento.endswith(":") for alinhamento in alinhamentos]

    partes = ["<table><thead><tr>"]
    for posicao, celula in enumerate(cabecalho):
        classe = ' class="direita"' if alinhadaADireita[posicao] else ""
        partes.append(f"<th{classe}>{formatarTextoDaLinha(celula)}</th>")
    partes.append("</tr></thead><tbody>")
    for linha in linhasDaTabela[2:]:
        partes.append("<tr>")
        for posicao, celula in enumerate(separarCelulas(linha)):
            classe = ' class="direita"' if posicao < len(alinhadaADireita) and alinhadaADireita[posicao] else ""
            partes.append(f"<td{classe}>{formatarTextoDaLinha(celula)}</td>")
        partes.append("</tr>")
    partes.append("</tbody></table>")
    return "".join(partes)


def converterMarkdownParaHtml(textoMarkdown):
    linhas = textoMarkdown.split("\n")
    partesDoHtml = []
    estamosNasReferencias = False
    i = 0

    while i < len(linhas):
        linha = linhas[i]
        linhaSemEspacos = linha.strip()

        if not linhaSemEspacos:
            i += 1
            continue

        # título principal: vai pra capa, não pro corpo
        if linha.startswith("# "):
            i += 1
            continue

        # seção: "## 1 Introdução" -> "1  INTRODUÇÃO"
        if linha.startswith("## "):
            titulo = linha[3:].strip()
            estamosNasReferencias = titulo.lower().startswith("refer")
            partes = titulo.split(" ", 1)
            if partes[0].isdigit():
                titulo = f"{partes[0]}&nbsp;&nbsp;{html.escape(partes[1])}"
            else:
                titulo = html.escape(titulo)
            partesDoHtml.append(f"<h2>{titulo}</h2>")
            i += 1
            continue

        # subseção: "### 4.1 E1: ..." -> "4.1  E1: ..."
        if linha.startswith("### "):
            titulo = linha[4:].strip()
            partes = titulo.split(" ", 1)
            titulo = f"{partes[0]}&nbsp;&nbsp;{formatarTextoDaLinha(partes[1])}" if len(partes) == 2 else titulo
            partesDoHtml.append(f"<h3>{titulo}</h3>")
            i += 1
            continue

        # bloco de código
        if linhaSemEspacos.startswith("```"):
            i += 1
            linhasDoCodigo = []
            while i < len(linhas) and not linhas[i].strip().startswith("```"):
                linhasDoCodigo.append(linhas[i])
                i += 1
            i += 1
            partesDoHtml.append(f"<pre>{html.escape(chr(10).join(linhasDoCodigo))}</pre>")
            continue

        # tabela
        if linhaSemEspacos.startswith("|"):
            linhasDaTabela = []
            while i < len(linhas) and linhas[i].strip().startswith("|"):
                linhasDaTabela.append(linhas[i])
                i += 1
            partesDoHtml.append(converterTabela(linhasDaTabela))
            continue

        # lista com marcador ou numerada
        ehListaComMarcador = linhaSemEspacos.startswith("- ")
        ehListaNumerada = re.match(r"^\d+\. ", linhaSemEspacos) is not None
        if ehListaComMarcador or ehListaNumerada:
            itens = []
            numeroInicial = int(linhaSemEspacos.split(".")[0]) if ehListaNumerada else 1
            while i < len(linhas):
                atual = linhas[i].strip()
                if ehListaComMarcador and atual.startswith("- "):
                    itens.append(atual[2:])
                elif ehListaNumerada and re.match(r"^\d+\. ", atual):
                    itens.append(atual.split(" ", 1)[1])
                else:
                    break
                i += 1
            etiqueta = "ul" if ehListaComMarcador else "ol"
            inicio = f' start="{numeroInicial}"' if ehListaNumerada and numeroInicial != 1 else ""
            corpo = "".join(f"<li>{formatarTextoDaLinha(item)}</li>" for item in itens)
            partesDoHtml.append(f"<{etiqueta}{inicio}>{corpo}</{etiqueta}>")
            continue

        # parágrafo (junta linhas seguidas)
        linhasDoParagrafo = []
        while i < len(linhas) and linhas[i].strip() and not linhas[i].startswith("#") \
                and not linhas[i].strip().startswith(("|", "```", "- ")) \
                and not re.match(r"^\d+\. ", linhas[i].strip()):
            linhasDoParagrafo.append(linhas[i].strip())
            i += 1
        classe = ' class="referencia"' if estamosNasReferencias else ""
        partesDoHtml.append(f"<p{classe}>{formatarTextoDaLinha(' '.join(linhasDoParagrafo))}</p>")

    return "\n".join(partesDoHtml)


def montarCapa(titulo):
    capa = DADOS_DA_CAPA
    alunos = "".join(f"<p>{html.escape(nome)}</p>" for nome in capa["alunos"])
    return f"""
<section class="capa">
  <div><p>{capa['universidade']}</p><p>{capa['curso']}</p></div>
  <div><p class="titulo">{html.escape(titulo)}</p></div>
  <div>{alunos}<p>&nbsp;</p>
       <p>Disciplina: {capa['disciplina']}</p>
       <p>Professor: {capa['professor']}</p>
       <p>Semestre: {capa['semestre']}</p></div>
  <div><p>{capa['cidade']}</p><p>{capa['ano']}</p></div>
</section>"""


def main():
    textoMarkdown = ARQUIVO_MARKDOWN.read_text(encoding="utf-8")
    titulo = next(linha[2:].strip() for linha in textoMarkdown.split("\n") if linha.startswith("# "))

    documentoHtml = f"""<!doctype html>
<html lang="pt-BR"><head><meta charset="utf-8"><title>{html.escape(titulo)}</title>
<style>{ESTILO}</style></head>
<body>{montarCapa(titulo)}
{converterMarkdownParaHtml(textoMarkdown)}
</body></html>"""

    with tempfile.TemporaryDirectory() as pastaTemporaria:
        arquivoHtml = Path(pastaTemporaria) / "relatorio.html"
        arquivoHtml.write_text(documentoHtml, encoding="utf-8")
        comando = [CAMINHO_DO_CHROME, "--headless=new", "--disable-gpu", "--no-pdf-header-footer",
                   f"--print-to-pdf={ARQUIVO_PDF}", arquivoHtml.as_uri()]
        resultado = subprocess.run(comando, capture_output=True, text=True)
        if resultado.returncode != 0 or not ARQUIVO_PDF.exists():
            print(resultado.stderr, file=sys.stderr)
            sys.exit("ERRO: o Chrome nao conseguiu gerar o PDF")
    print(f"PDF gerado em {ARQUIVO_PDF.relative_to(PASTA_DO_PROJETO)}")


if __name__ == "__main__":
    main()

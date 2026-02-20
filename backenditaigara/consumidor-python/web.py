from fastapi import FastAPI, Request
from fastapi.responses import HTMLResponse, RedirectResponse, Response
from fastapi.templating import Jinja2Templates
from client import LaudosClient
import base64
import os

app = FastAPI(title="Python Consumer TUI Web")

templates = Jinja2Templates(directory=os.path.join(os.path.dirname(__file__), "templates"))

# Instancia o cliente da API
api_client = LaudosClient(
    base_url="http://localhost:8000",
    client_id="A8iSuj9dX0aIcp50ENpCT7DIcl9N0ZFj",
    client_secret="a^zcBXi<w<,V1ml?)vOpLgQ$Pp<t>*uOOl9WV"
)

@app.get("/", response_class=RedirectResponse)
async def home():
    return RedirectResponse(url="/buscar")

@app.get("/buscar", response_class=HTMLResponse)
async def buscar(request: Request, cpf: str = None):
    # Se não houver CPF na busca
    if not cpf:
        return templates.TemplateResponse("index.html", {
            "request": request,
            "error": "Por favor, informe um CPF na URL (ex: /buscar?cpf=12345678903)",
            "laudos": []
        })

    # Limpar traços e pontuação igual no Go
    cpf_limpo = cpf.replace(".", "").replace("-", "")

    try:
        laudos = api_client.listar_resultados_por_cpf(cpf_limpo)
        return templates.TemplateResponse("index.html", {
            "request": request,
            "error": None,
            "laudos": laudos
        })
    except Exception as e:
        return templates.TemplateResponse("index.html", {
            "request": request,
            "error": str(e),
            "laudos": []
        })

@app.get("/laudo")
async def ver_laudo(cpf: str, accession: str):
    if not cpf or not accession:
        return Response("Requisição inválida", status_code=400)

    try:
        laudos = api_client.listar_resultados_por_cpf(cpf)
        target_pdf = None
        for l in laudos:
            if l.get("accession_number") == accession:
                target_pdf = l.get("pdf_base64")
                break
        
        if not target_pdf:
            return Response("Laudo não encontrado ou PDF indisponível", status_code=404)

        pdf_bytes = base64.b64decode(target_pdf)
        headers = {
            "Content-Type": "application/pdf",
            "Content-Disposition": f'inline; filename="laudo_{accession}.pdf"'
        }
        return Response(content=pdf_bytes, headers=headers)
    except Exception as e:
        return Response(f"Erro no servidor: {str(e)}", status_code=500)


if __name__ == "__main__":
    import uvicorn
    # Inicializando na porta alternativa como pedido pelo usuário
    print("Iniciando Consumer Python Web Server na porta 8081...")
    uvicorn.run("web:app", host="0.0.0.0", port=8081, reload=True)

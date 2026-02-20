from fastapi import FastAPI, Depends, HTTPException, status
from fastapi.security import OAuth2PasswordBearer, OAuth2PasswordRequestForm
from datetime import datetime, timedelta
from typing import Optional
from jose import JWTError, jwt
import json
import os

app = FastAPI(title="Laudos API Mock")

# Configurações do JWT
SECRET_KEY = "dummy_secret_key_for_mock_api_only"
ALGORITHM = "HS256"
ACCESS_TOKEN_EXPIRE_MINUTES = 30

# Credenciais simuladas (conforme seu exemplo)
VALID_CLIENT_ID = "A8iSuj9dX0aIcp50ENpCT7DIcl9N0ZFj"
VALID_CLIENT_SECRET = "a^zcBXi<w<,V1ml?)vOpLgQ$Pp<t>*uOOl9WV"

oauth2_scheme = OAuth2PasswordBearer(tokenUrl="token")

# Carregamento do DB em memória
LAUDOS_DB = {}
db_path = os.path.join(os.path.dirname(__file__), "laudos_db.json")

@app.on_event("startup")
async def startup_event():
    global LAUDOS_DB
    if os.path.exists(db_path):
        with open(db_path, "r", encoding="utf-8") as f:
            LAUDOS_DB = json.load(f)
    print(f"Banco de laudos carregado. {len(LAUDOS_DB)} pacientes encontrados.")

def create_access_token(data: dict, expires_delta: Optional[timedelta] = None):
    to_encode = data.copy()
    if expires_delta:
        expire = datetime.utcnow() + expires_delta
    else:
        expire = datetime.utcnow() + timedelta(minutes=15)
    to_encode.update({"exp": expire})
    encoded_jwt = jwt.encode(to_encode, SECRET_KEY, algorithm=ALGORITHM)
    return encoded_jwt

@app.post("/token")
async def login_for_access_token(form_data: OAuth2PasswordRequestForm = Depends()):
    # Simula o Auth Client Credentials validando via form
    if form_data.username != VALID_CLIENT_ID or form_data.password != VALID_CLIENT_SECRET:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Incorrect client_id or client_secret",
            headers={"WWW-Authenticate": "Bearer"},
        )
    
    access_token_expires = timedelta(minutes=ACCESS_TOKEN_EXPIRE_MINUTES)
    access_token = create_access_token(
        data={"sub": form_data.username}, expires_delta=access_token_expires
    )
    return {"access_token": access_token, "token_type": "bearer", "expires_in": ACCESS_TOKEN_EXPIRE_MINUTES * 60}

async def get_current_client(token: str = Depends(oauth2_scheme)):
    credentials_exception = HTTPException(
        status_code=status.HTTP_401_UNAUTHORIZED,
        detail="Could not validate credentials",
        headers={"WWW-Authenticate": "Bearer"},
    )
    try:
        payload = jwt.decode(token, SECRET_KEY, algorithms=[ALGORITHM])
        client_id: str = payload.get("sub")
        if client_id is None:
            raise credentials_exception
    except JWTError:
        raise credentials_exception
    return client_id

@app.get("/laudos/{cpf}")
async def get_laudos(cpf: str, current_client: str = Depends(get_current_client)):
    # Remove máscaras para garantir a busca correta caso seja passado pontuado
    clean_cpf = cpf.replace(".", "").replace("-", "")
    
    if clean_cpf not in LAUDOS_DB:
         raise HTTPException(status_code=404, detail="Nenhum laudo encontrado para este CPF")
         
    resultados = LAUDOS_DB[clean_cpf]
    
    # Opcional: injetar dados fake do paciente baseando-se no CPF pra emular o seu log
    for r in resultados:
        if "nr_cpf" not in r:
            r["nr_cpf"] = clean_cpf
        if "nm_pessoa" not in r:
            r["nm_pessoa"] = "Paciente Teste"
            
    return {"resultados": resultados}

if __name__ == "__main__":
    import uvicorn
    uvicorn.run("main:app", host="0.0.0.0", port=8000, reload=True)

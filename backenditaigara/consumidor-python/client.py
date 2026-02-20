import requests
import time

class LaudosClient:
    def __init__(self, base_url, client_id, client_secret):
        self.base_url = base_url
        self.credentials = {
            "username": client_id,     # OAuth2PasswordRequestForm expects username
            "password": client_secret, # OAuth2PasswordRequestForm expects password
            "grant_type": "password"
        }
        self.access_token = None
        self.token_expiry = 0

    def _authenticate(self):
        print("Obtendo novo token de acesso...")
        token_url = f"{self.base_url}/token"
        response = requests.post(token_url, data=self.credentials)
        if response.status_code == 200:
            data = response.json()
            self.access_token = data.get("access_token")
            expires_in = data.get("expires_in", 1800)
            self.token_expiry = time.time() + expires_in - 60 # 1 minuto de margem
            print("Autenticação bem sucedida!")
        else:
            raise Exception(f"Falha na autenticação: {response.status_code} - {response.text}")

    def _get_vital_token(self):
        # Renova o token se estiver faltando menos de 1 margem ou não existir
        if not self.access_token or time.time() >= self.token_expiry:
            self._authenticate()
        return self.access_token

    def listar_resultados_por_cpf(self, cpf):
        token = self._get_vital_token()
        headers = {
            "Authorization": f"Bearer {token}"
        }
        
        url = f"{self.base_url}/laudos/{cpf}"
        print(f"Buscando CPF {cpf}...")
        response = requests.get(url, headers=headers)
        
        if response.status_code == 200:
            return response.json().get("resultados", [])
        elif response.status_code == 404:
            return []
        else:
            print(f"Erro inexperado: {response.status_code} - {response.text}")
            return []


# Exemplo de uso 
if __name__ == "__main__":
    client = LaudosClient(
        base_url="http://localhost:8000",
        client_id="A8iSuj9dX0aIcp50ENpCT7DIcl9N0ZFj",
        client_secret="a^zcBXi<w<,V1ml?)vOpLgQ$Pp<t>*uOOl9WV"
    )

    print("--- TESTE PACIENTE COM DADOS ---")
    resultados = client.listar_resultados_por_cpf("12345678903")
    if not resultados:
        print("Nenhum laudo encontrado para este CPF.")
    else:
        primeiro = resultados[0]
        cpf_encontrado = primeiro.get("nr_cpf", "12345678903")
        nome = primeiro.get("nm_pessoa", "Paciente")
        print(f"Paciente: {nome} | CPF: {cpf_encontrado}")
        print(f"Total de {len(resultados)} exames encontrados:")
        for r in resultados:
            print(f"- {r.get('data')} | {r.get('tipo')} [{r.get('accession_number')}]")

    print("\n--- TESTE PACIENTE SEM DADOS ---")
    resultados = client.listar_resultados_por_cpf("00000000000")
    if not resultados:
        print("Nenhum laudo encontrado para este CPF (Status 404 esperado).")
    else:
        print(f"Resultados: {resultados}")

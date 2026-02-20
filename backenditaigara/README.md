# Itaigara Clinic System - Backend & Consumidores

Bem-vindo à documentação oficial do ecossistema de testes do Itaigara Clinic System. 

Este repositório é composto por **três aplicações independentes** criadas para simular o fluxo de distribuição confiável de laudos médicos em formato PDF. Este projeto atende às necessidades de testes para clientes internos e externos na modalidade Machine-To-Machine e interfaces TUI/Web.

---

## 🏗️ Visão Geral da Arquitetura

1. **API Mock (`/api-mock`)**
   - **Framework:** FastAPI (Python)
   - **Papel:** Simula um backend legado hospitalar provendo autenticação OAuth2 e listagem de exames e laudos a partir de um pseudo banco-de-dados local.

2. **Consumidor em Go (`/consumidor-go`)**
   - **Framework:** Go Nativo + Bubbletea + html/template
   - **Papel:** Apresenta uma tela interativa pelo terminal e, ao mesmo tempo, inicializa um servidor Frontend leve de fundo (`http://localhost:8080`) capaz de processar PDFs em memória. Possui gerenciador de estado (`config.json`) para persistência de credenciais OAuth2.

3. **Consumidor em Python (`/consumidor-python`)**
   - **Framework:** FastAPI + Jinja2 + Requests
   - **Papel:** Alternativa ao consumidor em Go e prova-de-conceito construída em Python demonstrando a mesma funcionalidade TUI de carregamento e leitura de PDF via web e CLI (`http://localhost:8081`).

---

## 🔒 1. Passo a Passo: API Mock (Servidor Base)

O primeiro serviço a ser iniciado obrigatoriamente é a API Mock (porta `8000`), pois ela é o "core" que as outras aplicações irão consultar.

### Configuração:
1. Navegue até a pasta corporativa:
   ```bash
   cd /Users/raunick/Downloads/projetos/backenditaigara/api-mock
   ```
2. Crie e ative o ambiente virtual:
   ```bash
   python3 -m venv .venv
   source .venv/bin/activate
   ```
3. Instale as dependências:
   ```bash
   pip install -r requirements.txt
   ```

### Execução:
Você pode rodar no terminal conectado (para ver os logs em tempo real) ou em background (para que não ocupe uma aba do seu console).
*   **Tempo Real / Desenvolvimento:** `uvicorn main:app --port 8000 --reload`
*   **Plano de Fundo (Recomendado):** `nohup uvicorn main:app --port 8000 > api.log 2>&1 &`

---

## 🚀 2. Passo a Passo: Consumidor Go (Terminal + Web)

Este consumidor conta com uma interface no terminal guiada e autoconfigurável.

### Configuração (Primeira Execução):
1. Navegue para a pasta do consumidor:
   ```bash
   cd /Users/raunick/Downloads/projetos/backenditaigara/consumidor-go
   ```
2. Instale e baixe os pacotes da build inicial:
   ```bash
   go mod tidy
   ```
3. Execute o programa:
   ```bash
   go run .
   ```

### Fluxo de Setup Interativo (TUI):
Na primeira execução, ele notará a falta do `config.json` e abrirá a tela preta com o cabeçalho azul perguntando por suas configurações mestre (Credenciais Mock):
- **Base URL:** Escreva `http://localhost:8000`
- **Client ID:** Copie e cole a chave disponibilizada (`A8iSuj9dX0a...`)
- **Client Secret:** Copie e cole o segredo disponilizado (`a^zcBXi<...`)

Pressione `Enter` para salvar (ou mude entre os campos usando as `"Setas Pra Cima"` / `"Setas Pra Baixo"`).
> O arquivo `config.json` será salvo automaticamente. Nas próximas inicializações, esse passo será ocultado, caindo direto na pesquisa de CPF.

### Uso do Frontend WEB:
Se não quiser usar a TUI do terminal, deixe ele aberto com a mensagem `Servidor Web rodando em http://localhost:8080` e acesso do seu navegador:
- [Página de Busca (Go)](http://localhost:8080/buscar)

---

## 🐍 3. Passo a Passo: Consumidor Python (Backend Alternativo)

Serviço concorrente (roda sob porta `8081`) e pode ser estendido facilmente pelo seu analista Python.

### Configuração:
1. Abra o respectivo diretório:
   ```bash
   cd /Users/raunick/Downloads/projetos/backenditaigara/consumidor-python
   ```
2. Inicialize o contêiner virtual e instale:
   ```bash
   python3 -m venv .venv
   source .venv/bin/activate
   pip install -r requirements.txt
   ```

### Uso do Terminal Integrado (Scripting):
Para usar os dados por trás das cortinas no console de saída do Python, rode o client isolado:
```bash
python3 client.py
```

### Uso do Frontend WEB (Jinja2):
Para testar os layouts de laudos do Python no seu próprio navegador:
*   **Tempo Real:** `uvicorn web:app --port 8081 --reload`
*   **Acesso Web:** Navegue para [http://localhost:8081/buscar](http://localhost:8081/buscar)

---

## 🧪 Postman Collection (Testes Frios)
Na pasta `/api-mock/`, você tem o arquivo de Collection do Postman `Itaigara_API_Mock.postman_collection.json`. 

- Abra o Postman > Clique em **Import** (File > Import) e arraste o arquivo para dentro.
- Ele instalará a collection **Itaigara - Laudos API Mock** contendo as rotas exatas, passando por `POST /token` e enviando os Bearer Tokens automaticamente para buscar o laudo final, simulando a inteligência nativa dos consumidores criados.

---
> ⚕️ **Itaigara Clinic System - Engineering Labs** - 2026

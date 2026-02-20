package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"net/url"
	"sync"
	"time"
)

type Laudo struct {
	AccessionNumber string `json:"accession_number"`
	Tipo            string `json:"tipo"`
	Descricao       string `json:"descricao"`
	Data            string `json:"data"`
	PdfBase64       string `json:"pdf_base64"`
	NrCpf           string `json:"nr_cpf,omitempty"`
	NmPessoa        string `json:"nm_pessoa,omitempty"`
}

type LaudosResponse struct {
	Resultados []Laudo `json:"resultados"`
}

type TokenResponse struct {
	AccessToken string `json:"access_token"`
	TokenType   string `json:"token_type"`
	ExpiresIn   int    `json:"expires_in"`
}

type LaudosClient struct {
	baseURL      string
	clientID     string
	clientSecret string
	token        string
	tokenExpiry  time.Time
	mu           sync.Mutex
	httpClient   *http.Client
}

func NewLaudosClient(baseURL, clientID, clientSecret string) *LaudosClient {
	return &LaudosClient{
		baseURL:      baseURL,
		clientID:     clientID,
		clientSecret: clientSecret,
		httpClient:   &http.Client{Timeout: 10 * time.Second},
	}
}

func (c *LaudosClient) authenticate() error {
	data := url.Values{}
	data.Set("grant_type", "password")
	data.Set("username", c.clientID)
	data.Set("password", c.clientSecret)

	req, err := http.NewRequest("POST", c.baseURL+"/token", bytes.NewBufferString(data.Encode()))
	if err != nil {
		return err
	}
	req.Header.Set("Content-Type", "application/x-www-form-urlencoded")

	resp, err := c.httpClient.Do(req)
	if err != nil {
		return err
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		body, _ := io.ReadAll(resp.Body)
		return fmt.Errorf("falha na autenticação: status %d - %s", resp.StatusCode, string(body))
	}

	var tokenResp TokenResponse
	if err := json.NewDecoder(resp.Body).Decode(&tokenResp); err != nil {
		return err
	}

	c.token = tokenResp.AccessToken
	// Margin de segurança de 1 minuto
	c.tokenExpiry = time.Now().Add(time.Duration(tokenResp.ExpiresIn-60) * time.Second)
	return nil
}

func (c *LaudosClient) getValidToken() (string, error) {
	c.mu.Lock()
	defer c.mu.Unlock()

	if c.token == "" || time.Now().After(c.tokenExpiry) {
		if err := c.authenticate(); err != nil {
			return "", err
		}
	}
	return c.token, nil
}

func (c *LaudosClient) BuscarLaudos(cpf string) ([]Laudo, error) {
	token, err := c.getValidToken()
	if err != nil {
		return nil, fmt.Errorf("erro ao obter token: %v", err)
	}

	req, err := http.NewRequest("GET", fmt.Sprintf("%s/laudos/%s", c.baseURL, cpf), nil)
	if err != nil {
		return nil, err
	}
	req.Header.Set("Authorization", "Bearer "+token)

	resp, err := c.httpClient.Do(req)
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()

	if resp.StatusCode == http.StatusNotFound {
		return []Laudo{}, nil // CPF não encontrado, retorna lista vazia
	}

	if resp.StatusCode != http.StatusOK {
		body, _ := io.ReadAll(resp.Body)
		return nil, fmt.Errorf("erro na api: status %d - %s", resp.StatusCode, string(body))
	}

	var laudosResp LaudosResponse
	if err := json.NewDecoder(resp.Body).Decode(&laudosResp); err != nil {
		return nil, err
	}

	return laudosResp.Resultados, nil
}

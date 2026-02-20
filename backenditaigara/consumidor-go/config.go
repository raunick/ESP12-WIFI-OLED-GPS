package main

import (
	"encoding/json"
	"os"
	"path/filepath"
)

type Config struct {
	BaseURL      string `json:"base_url"`
	ClientID     string `json:"client_id"`
	ClientSecret string `json:"client_secret"`
}

var configFilePath = "config.json"

// LoadConfig tenta carregar a configuração do arquivo JSON local.
func LoadConfig() (Config, error) {
	var cfg Config

	if _, err := os.Stat(configFilePath); os.IsNotExist(err) {
		return cfg, err // Arquivo não existe
	}

	data, err := os.ReadFile(configFilePath)
	if err != nil {
		return cfg, err
	}

	err = json.Unmarshal(data, &cfg)
	return cfg, err
}

// SaveConfig salva a configuração atual no arquivo JSON local.
func SaveConfig(cfg Config) error {
	data, err := json.MarshalIndent(cfg, "", "  ")
	if err != nil {
		return err
	}

	// Garante que o diretório exista caso seja movido
	dir := filepath.Dir(configFilePath)
	if err := os.MkdirAll(dir, 0755); err != nil {
		return err
	}

	return os.WriteFile(configFilePath, data, 0644)
}

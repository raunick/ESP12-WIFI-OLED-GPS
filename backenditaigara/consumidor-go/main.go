package main

import (
	"encoding/base64"
	"fmt"
	"html/template"
	"log"
	"net/http"
	"os"
	"strings"
	"time"

	"github.com/charmbracelet/bubbles/textinput"
	tea "github.com/charmbracelet/bubbletea"
	"github.com/charmbracelet/lipgloss"
)

var (
	apiClient        *LaudosClient
	appConfig        Config
	webServerStarted bool

	// Estilos do Bubbletea
	titleStyle = lipgloss.NewStyle().
			Bold(true).
			Foreground(lipgloss.Color("#FFFFFF")).
			Background(lipgloss.Color("#2563EB")).
			Padding(0, 1).
			MarginBottom(1)

	promptStyle = lipgloss.NewStyle().Foreground(lipgloss.Color("#10B981")).Bold(true)
	errorStyle  = lipgloss.NewStyle().Foreground(lipgloss.Color("#EF4444"))
	infoStyle   = lipgloss.NewStyle().Foreground(lipgloss.Color("#8B5CF6"))
	itemStyle   = lipgloss.NewStyle().PaddingLeft(2)
)

// --- Servidor Web ---

type PageData struct {
	CPF    string
	Laudos []Laudo
	Error  string
}

func startHTTPServer() {
	tmpl := template.Must(template.ParseGlob("templates/*.html"))

	http.HandleFunc("/buscar", func(w http.ResponseWriter, r *http.Request) {
		cpf := r.URL.Query().Get("cpf")
		cpf = strings.ReplaceAll(strings.ReplaceAll(cpf, ".", ""), "-", "")

		data := PageData{CPF: cpf}

		if cpf == "" {
			data.Error = "Por favor, informe um CPF na URL (ex: /buscar?cpf=12345678903)"
			tmpl.ExecuteTemplate(w, "index.html", data)
			return
		}

		laudos, err := apiClient.BuscarLaudos(cpf)
		if err != nil {
			data.Error = err.Error()
		} else {
			data.Laudos = laudos
		}

		err = tmpl.ExecuteTemplate(w, "index.html", data)
		if err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
		}
	})

	http.HandleFunc("/laudo", func(w http.ResponseWriter, r *http.Request) {
		cpf := r.URL.Query().Get("cpf")
		accession := r.URL.Query().Get("accession")

		if cpf == "" || accession == "" {
			http.Error(w, "Requisição inválida", http.StatusBadRequest)
			return
		}

		laudos, err := apiClient.BuscarLaudos(cpf)
		if err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}

		var targetPdf string
		for _, l := range laudos {
			if l.AccessionNumber == accession {
				targetPdf = l.PdfBase64
				break
			}
		}

		if targetPdf == "" {
			http.Error(w, "Laudo não encontrado ou PDF indisponível", http.StatusNotFound)
			return
		}

		pdfBytes, err := base64.StdEncoding.DecodeString(targetPdf)
		if err != nil {
			http.Error(w, "Erro ao decodificar arquivo", http.StatusInternalServerError)
			return
		}

		w.Header().Set("Content-Type", "application/pdf")
		w.Header().Set("Content-Disposition", "inline; filename=\"laudo_"+accession+".pdf\"")
		w.Write(pdfBytes)
	})

	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		// Redireciona raiz para buscar com CPF em branco para mostrar o form
		http.Redirect(w, r, "/buscar", http.StatusSeeOther)
	})

	log.Println("Servidor Web rodando em http://localhost:8080")
	if err := http.ListenAndServe(":8080", nil); err != nil {
		log.Fatal(err)
	}
}

func initApp() {
	apiClient = NewLaudosClient(appConfig.BaseURL, appConfig.ClientID, appConfig.ClientSecret)
	if !webServerStarted {
		webServerStarted = true
		go startHTTPServer()
	}
}

// --- BubbleTea TUI ---

type appState int

const (
	stateConfig appState = iota
	stateSearch
)

type model struct {
	state appState

	// Config state
	inputs  []textinput.Model
	focused int

	// Search state
	cpfInput string
	loading  bool
	laudos   []Laudo
	err      error
	searched bool
}

type laudosMsg struct {
	laudos []Laudo
	err    error
}

func initialModel() model {
	m := model{
		state:    stateSearch,
		cpfInput: "",
		loading:  false,
	}

	cfg, err := LoadConfig()
	if err != nil || cfg.BaseURL == "" || cfg.ClientID == "" || cfg.ClientSecret == "" {
		m.state = stateConfig
		m.inputs = make([]textinput.Model, 3)

		var t textinput.Model
		for i := range m.inputs {
			t = textinput.New()
			t.CharLimit = 150

			switch i {
			case 0:
				t.Placeholder = "Base URL (ex: http://localhost:8000)"
				t.Focus()
				t.PromptStyle = lipgloss.NewStyle().Foreground(lipgloss.Color("#10B981"))
				t.TextStyle = lipgloss.NewStyle().Foreground(lipgloss.Color("#10B981"))
				if cfg.BaseURL != "" {
					t.SetValue(cfg.BaseURL)
				}
			case 1:
				t.Placeholder = "Client ID"
				if cfg.ClientID != "" {
					t.SetValue(cfg.ClientID)
				}
			case 2:
				t.Placeholder = "Client Secret"
				if cfg.ClientSecret != "" {
					t.SetValue(cfg.ClientSecret)
				}
			}
			m.inputs[i] = t
		}
	} else {
		appConfig = cfg
		initApp()
	}

	return m
}

func (m model) Init() tea.Cmd {
	return textinput.Blink
}

func (m model) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	switch msg := msg.(type) {
	case tea.KeyMsg:
		switch msg.Type {
		case tea.KeyCtrlC, tea.KeyEsc:
			return m, tea.Quit
		}
	}

	if m.state == stateConfig {
		return updateConfig(m, msg)
	}
	return updateSearch(m, msg)
}

func updateConfig(m model, msg tea.Msg) (tea.Model, tea.Cmd) {
	switch msg := msg.(type) {
	case tea.KeyMsg:
		switch msg.Type {
		case tea.KeyEnter, tea.KeyUp, tea.KeyDown:
			s := msg.String()

			// Se enter no último campo, tenta salvar config e iniciar app
			if s == "enter" && m.focused == len(m.inputs)-1 {
				appConfig = Config{
					BaseURL:      m.inputs[0].Value(),
					ClientID:     m.inputs[1].Value(),
					ClientSecret: m.inputs[2].Value(),
				}
				SaveConfig(appConfig)
				initApp()
				m.state = stateSearch
				// Pequeno delay fake para UX
				time.Sleep(500 * time.Millisecond)
				return m, nil
			}

			// Navegação entre campos
			if s == "up" {
				m.focused--
			} else {
				m.focused++
			}

			if m.focused > len(m.inputs)-1 {
				m.focused = 0
			} else if m.focused < 0 {
				m.focused = len(m.inputs) - 1
			}

			cmds := make([]tea.Cmd, len(m.inputs))
			for i := 0; i <= len(m.inputs)-1; i++ {
				if i == m.focused {
					cmds[i] = m.inputs[i].Focus()
					m.inputs[i].PromptStyle = lipgloss.NewStyle().Foreground(lipgloss.Color("#10B981"))
					m.inputs[i].TextStyle = lipgloss.NewStyle().Foreground(lipgloss.Color("#10B981"))
				} else {
					m.inputs[i].Blur()
					m.inputs[i].PromptStyle = lipgloss.NewStyle().Foreground(lipgloss.Color("#6B7280"))
					m.inputs[i].TextStyle = lipgloss.NewStyle().Foreground(lipgloss.Color("#6B7280"))
				}
			}
			return m, tea.Batch(cmds...)
		}
	}

	// Update the focused text input
	cmds := make([]tea.Cmd, len(m.inputs))
	for i := range m.inputs {
		m.inputs[i], cmds[i] = m.inputs[i].Update(msg)
	}
	return m, tea.Batch(cmds...)
}

func updateSearch(m model, msg tea.Msg) (tea.Model, tea.Cmd) {
	switch msg := msg.(type) {
	case tea.KeyMsg:
		switch msg.Type {
		case tea.KeyBackspace:
			if len(m.cpfInput) > 0 {
				m.cpfInput = m.cpfInput[:len(m.cpfInput)-1]
			}
		case tea.KeyEnter:
			if m.cpfInput != "" {
				m.loading = true
				m.searched = false
				m.err = nil
				cpf := m.cpfInput
				return m, func() tea.Msg {
					laudos, err := apiClient.BuscarLaudos(cpf)
					return laudosMsg{laudos: laudos, err: err}
				}
			}
		case tea.KeyRunes:
			// Só aceita numeros
			if msg.String() >= "0" && msg.String() <= "9" && len(m.cpfInput) < 11 {
				m.cpfInput += msg.String()
			}
		}

	case laudosMsg:
		m.loading = false
		m.searched = true
		if msg.err != nil {
			m.err = msg.err
		} else {
			m.laudos = msg.laudos
		}
	}

	return m, nil
}

func viewConfig(m model) string {
	var b strings.Builder
	b.WriteString(titleStyle.Render(" ITAIGARA CLINIC SYSTEM - Initial Setup "))
	b.WriteString("\n\nNenhuma configuração encontrada ou configuração incompleta.\nPor favor, preencha as credenciais da API.\n\n")

	for i := range m.inputs {
		b.WriteString(m.inputs[i].View())
		if i < len(m.inputs)-1 {
			b.WriteRune('\n')
		}
	}

	b.WriteString("\n\n" + lipgloss.NewStyle().Foreground(lipgloss.Color("#6B7280")).Render("(Use ↑/↓ para navegar e ENTER no último campo para salvar e avançar)"))
	return b.String()
}

func viewSearch(m model) string {
	var b strings.Builder

	b.WriteString(titleStyle.Render(" ITAIGARA CLINIC SYSTEM - Consumidor TUI "))
	b.WriteString("\n\nServidor Web Frontend rodando em: " + infoStyle.Render("http://localhost:8080/buscar"))
	b.WriteString("\n\n")

	b.WriteString(promptStyle.Render("Digite o CPF (somente números, ENTER para buscar): "))
	b.WriteString(m.cpfInput)

	// Cursor artificial
	if len(m.cpfInput) < 11 {
		b.WriteString("█")
	}
	b.WriteString("\n\n")

	if m.loading {
		b.WriteString(infoStyle.Render("Buscando laudos na API Mock... Aguarde."))
		b.WriteString("\n")
	} else if m.searched {
		if m.err != nil {
			b.WriteString(errorStyle.Render(fmt.Sprintf("Erro ao buscar: %v", m.err)))
		} else if len(m.laudos) == 0 {
			b.WriteString(errorStyle.Render(" Nenhum laudo encontrado para este CPF (Status 404)."))
		} else {
			b.WriteString(infoStyle.Render(fmt.Sprintf(" Encontrados %d exames para o paciente. \n", len(m.laudos))))
			b.WriteString(strings.Repeat("-", 50) + "\n")
			for _, l := range m.laudos {
				b.WriteString(itemStyle.Render(fmt.Sprintf("• [%s] %s", l.Data, lipgloss.NewStyle().Bold(true).Render(l.Tipo))))
				b.WriteString("\n")
				b.WriteString(itemStyle.Render(fmt.Sprintf("  Desc: %s (Accession: %s)", l.Descricao, l.AccessionNumber)))
				b.WriteString("\n\n")
			}
		}
		b.WriteString("\n(Pressione Backspace para apagar e buscar outro, ou ESC para sair)\n")
	} else {
		b.WriteString(lipgloss.NewStyle().Foreground(lipgloss.Color("#6B7280")).Render("Dicas de teste: '12345678903' (com dados) ou '000' (sem dados)"))
		b.WriteString("\n(Pressione ESC para sair)\n")
	}

	return b.String()
}

func (m model) View() string {
	if m.state == stateConfig {
		return viewConfig(m)
	}
	return viewSearch(m)
}

func main() {
	p := tea.NewProgram(initialModel(), tea.WithAltScreen())
	if _, err := p.Run(); err != nil {
		fmt.Printf("Erro ao iniciar interface TUI: %v\n", err)
		os.Exit(1)
	}
}

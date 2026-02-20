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

	tea "github.com/charmbracelet/bubbletea"
	"github.com/charmbracelet/lipgloss"
)

var (
	apiClient *LaudosClient
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

// --- BubbleTea TUI ---

type model struct {
	cpfInput    string
	loading     bool
	laudos      []Laudo
	err         error
	searched    bool
	serverReady bool
}

type laudosMsg struct {
	laudos []Laudo
	err    error
}

func initialModel() model {
	return model{
		cpfInput: "",
		loading:  false,
	}
}

func (m model) Init() tea.Cmd {
	return tea.Batch(
		tea.Tick(time.Second, func(t time.Time) tea.Msg {
			return nil // Apenas pra forçar renderização inicial rápida
		}),
	)
}

func (m model) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	switch msg := msg.(type) {
	case tea.KeyMsg:
		switch msg.Type {
		case tea.KeyCtrlC, tea.KeyEsc:
			return m, tea.Quit
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

func (m model) View() string {
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

func main() {
	// 1. Inicializa o cliente HTTP interno que conversa com o FastAPI
	apiClient = NewLaudosClient(
		"http://localhost:8000",
		"A8iSuj9dX0aIcp50ENpCT7DIcl9N0ZFj",
		"a^zcBXi<w<,V1ml?)vOpLgQ$Pp<t>*uOOl9WV",
	)

	// 2. Inicia o servidor Web numa goroutine para rodar em backgorund
	go startHTTPServer()

	// 3. Aguarda 1 segundinho pra dar tempo do print do log web aparecer antes do TUI
	time.Sleep(500 * time.Millisecond)

	// 4. Inicia a interface rica no Terminal
	p := tea.NewProgram(initialModel(), tea.WithAltScreen())
	if _, err := p.Run(); err != nil {
		fmt.Printf("Erro ao iniciar interface TUI: %v", err)
		os.Exit(1)
	}
}

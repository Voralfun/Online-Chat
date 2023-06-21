using System;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Windows.Forms;

namespace ClientSocket
{
    public partial class Form1 : Form
    {
        TcpClient _client;
        byte[] _buffer = new byte[4096];
        bool _isConnected = false;
        bool _isConnecting = false;
        string _username = string.Empty;
        bool _joinedChat = false;

        public Form1()
        {
            InitializeComponent();
        }

        private void ConnectToServer()
        {
            // Отображение окна ввода имени пользователя
            InputUsernameForm inputForm = new InputUsernameForm();
            if (inputForm.ShowDialog() == DialogResult.OK)
            {
                _username = inputForm.Username;
            }
            else
            {
                // Пользователь отменил ввод имени, прерываем процесс подключения
                _isConnecting = false;
                return;
            }

            try
            {
                _client = new TcpClient();
                _client.Connect("127.0.0.1", 8855);

                // Отправка сообщения о присоединении к серверу
                string joinMessage = "Пользователь " + _username + " присоединился к чату.";
                var msg = Encoding.UTF8.GetBytes(joinMessage);
                _client.GetStream().Write(msg, 0, msg.Length);

                // Отображение приветственного сообщения
                string welcomeMessage = "Добро пожаловать, " + _username + "!";
                BeginInvoke((Action)(() =>
                {
                    listBox1.Items.Add(welcomeMessage);
                    listBox1.SelectedIndex = listBox1.Items.Count - 1;
                }));

                Thread receiveThread = new Thread(ReceiveMessages);
                receiveThread.Start();

                // Обновление пользовательского интерфейса должно выполняться на основном потоке
                BeginInvoke((Action)(() =>
                {
                    buttonConnect.Enabled = false;
                    buttonDisconnect.Enabled = true;
                    buttonSend.Enabled = true;
                    textBox1.Focus();
                }));

                _isConnected = true;
                _isConnecting = false;
                _joinedChat = true;
            }
            catch (Exception)
            {
                // Ошибка подключения к серверу, не делаем никаких дополнительных действий
                _isConnecting = false;
            }
        }

        private void ReceiveMessages()
        {
            while (_isConnected)
            {
                try
                {
                    var bytesIn = _client.GetStream().Read(_buffer, 0, _buffer.Length);
                    if (bytesIn <= 0)
                    {
                        // Соединение с сервером было разорвано, выход из цикла
                        break;
                    }

                    var str = Encoding.UTF8.GetString(_buffer, 0, bytesIn);

                    // Обновление пользовательского интерфейса должно выполняться на основном потоке
                    BeginInvoke((Action)(() =>
                    {
                        listBox1.Items.Add(str);
                        listBox1.SelectedIndex = listBox1.Items.Count - 1;
                    }));
                }
                catch (Exception)
                {
                    // Ошибка чтения данных от сервера, не делаем никаких дополнительных действий
                    break;
                }
            }

            // Соединение с сервером было разорвано, необходимо закрыть клиентский сокет
            _client.Close();

            // Обновление пользовательского интерфейса должно выполняться на основном потоке
            BeginInvoke((Action)(() =>
            {
                buttonConnect.Enabled = true;
                buttonDisconnect.Enabled = false;
                buttonSend.Enabled = false;
            }));
        }

        private void buttonConnect_Click(object sender, EventArgs e)
        {
            if (!_isConnected && !_isConnecting)
            {
                _isConnecting = true;

                Thread connectThread = new Thread(ConnectToServer);
                connectThread.Start();
            }
        }

        private void buttonDisconnect_Click(object sender, EventArgs e)
        {
            if (_isConnected)
            {
                if (_joinedChat)
                {
                    string disconnectMessage = "Пользователь " + _username + " вышел из чата.";
                    var msg = Encoding.UTF8.GetBytes(disconnectMessage);
                    _client.GetStream().Write(msg, 0, msg.Length);
                }

                _isConnected = false;
                _client.Close();

                buttonConnect.Enabled = true;
                buttonDisconnect.Enabled = false;
                buttonSend.Enabled = false;
            }
        }

        private void buttonSend_Click(object sender, EventArgs e)
        {
            if (_isConnected)
            {
                var msg = Encoding.UTF8.GetBytes(textBox1.Text);
                _client.GetStream().Write(msg, 0, msg.Length);

                textBox1.Text = "";
                textBox1.Focus();
            }
        }

        private void Form1_Load(object sender, EventArgs e)
        {

        }

        private void listBox1_SelectedIndexChanged(object sender, EventArgs e)
        {

        }
    }
}

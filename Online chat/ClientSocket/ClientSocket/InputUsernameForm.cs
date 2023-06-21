using System;
using System.Windows.Forms;

namespace ClientSocket
{
    public partial class InputUsernameForm : Form
    {
        public string Username { get; private set; }

        public InputUsernameForm()
        {
            InitializeComponent();
        }


        private void InputUsernameForm_Load(object sender, EventArgs e)
        {
           
        }

        private void button_ok_Click(object sender, EventArgs e)
        {
            if (!string.IsNullOrWhiteSpace(textBox2.Text))
            {
                Username = textBox2.Text;
                DialogResult = DialogResult.OK;
                Close();
            }
            else
            {
                MessageBox.Show("Введите ваше имя пользователя.", "Ошибка", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void button_cancel_Click(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
            Close();
        }

        private void label2_Click(object sender, EventArgs e)
        {

        }
    }
}

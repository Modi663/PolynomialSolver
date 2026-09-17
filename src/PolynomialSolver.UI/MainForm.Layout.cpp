#include "pch.h"
#include "MainForm.h"

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;

namespace PolynomialSolver::UI
{
    void MainForm::InitializeComponent()
    {
        Text = L"Полиномы";

        StartPosition =
            FormStartPosition::CenterScreen;

        ClientSize =
            Drawing::Size(1100, 720);

        MinimumSize =
            Drawing::Size(850, 590);

        BackColor =
            Color::FromArgb(245, 247, 251);

        Font =
            gcnew Drawing::Font(
                L"Segoe UI",
                10.0f);

        TableLayoutPanel^ mainLayout =
            gcnew TableLayoutPanel();

        mainLayout->Dock =
            DockStyle::Fill;

        mainLayout->Padding =
            System::Windows::Forms::Padding(24);

        mainLayout->ColumnCount = 1;
        mainLayout->RowCount = 4;

        mainLayout->RowStyles->Add(
            gcnew RowStyle(SizeType::AutoSize));

        mainLayout->RowStyles->Add(
            gcnew RowStyle(SizeType::AutoSize));

        mainLayout->RowStyles->Add(
            gcnew RowStyle(SizeType::AutoSize));

        mainLayout->RowStyles->Add(
            gcnew RowStyle(
                SizeType::Percent,
                100.0f));

        Label^ title = gcnew Label();

        title->Text = L"Полиномы";

        title->Font =
            gcnew Drawing::Font(
                L"Segoe UI",
                21.0f,
                FontStyle::Bold);

        title->ForeColor =
            Color::FromArgb(28, 37, 54);

        title->AutoSize = true;

        title->Margin =
            System::Windows::Forms::Padding(
                0, 0, 0, 20);

        FlowLayoutPanel^ degreeCard =
            gcnew FlowLayoutPanel();

        degreeCard->AutoSize = true;
        degreeCard->Dock = DockStyle::Fill;
        degreeCard->WrapContents = false;
        degreeCard->BackColor = Color::White;

        degreeCard->Padding =
            System::Windows::Forms::Padding(18);

        degreeCard->Margin =
            System::Windows::Forms::Padding(
                0, 0, 0, 14);

        Label^ degreeLabel =
            gcnew Label();

        degreeLabel->Text = L"Степень:";
        degreeLabel->AutoSize = true;

        degreeLabel->Margin =
            System::Windows::Forms::Padding(
                0, 7, 12, 0);

        degreeCard->Controls->Add(
            degreeLabel);

        degreeButtons_ = gcnew array<RadioButton^>(3);

        for (int degree = 1; degree <= 3; ++degree)
        {
            RadioButton^ button = gcnew RadioButton();

            button->Text = Convert::ToString(degree);
            button->Tag = degree;
            button->Appearance = Appearance::Button;
            button->FlatStyle = FlatStyle::Flat;
            button->FlatAppearance->BorderSize = 0;
            button->TextAlign = ContentAlignment::MiddleCenter;
            button->Size = Drawing::Size(82, 32);
            button->Margin =
                System::Windows::Forms::Padding(0, 0, 6, 0);
            button->BackColor = degree == 1
                ? Color::FromArgb(37, 99, 235)
                : Color::FromArgb(238, 242, 249);
            button->ForeColor = degree == 1
                ? Color::White
                : Color::FromArgb(47, 61, 84);

            degreeButtons_[degree - 1] = button;
            degreeCard->Controls->Add(button);
        }

        degreeButtons_[0]->Checked = true;

        for each (RadioButton ^ button in degreeButtons_)
        {
            button->CheckedChanged +=
                gcnew EventHandler(
                    this,
                    &MainForm::OnDegreeChanged);
        }

        TableLayoutPanel^ equationCard =
            gcnew TableLayoutPanel();

        equationCard->AutoSize = true;
        equationCard->Dock = DockStyle::Fill;
        equationCard->ColumnCount = 1;
        equationCard->RowCount = 3;
        equationCard->BackColor = Color::White;

        equationCard->Padding =
            System::Windows::Forms::Padding(18);

        equationCard->Margin =
            System::Windows::Forms::Padding(
                0, 0, 0, 14);

        Label^ equationTitle =
            gcnew Label();

        equationTitle->Text = L"Уравнение";
        equationTitle->AutoSize = true;

        equationTitle->Font =
            gcnew Drawing::Font(
                L"Segoe UI",
                11.0f,
                FontStyle::Bold);

        equationPanel_ =
            gcnew FlowLayoutPanel();

        equationPanel_->AutoSize = true;
        equationPanel_->Anchor = AnchorStyles::None;
        equationPanel_->WrapContents = false;

        equationPanel_->Margin =
            System::Windows::Forms::Padding(
                0, 16, 0, 16);

        FlowLayoutPanel^ buttons =
            gcnew FlowLayoutPanel();

        buttons->AutoSize = true;
        buttons->WrapContents = false;

        solveButton_ = gcnew Button();

        solveButton_->Text = L"Решить";

        solveButton_->Size =
            Drawing::Size(150, 38);

        solveButton_->BackColor =
            Color::FromArgb(37, 99, 235);

        solveButton_->ForeColor =
            Color::White;

        solveButton_->FlatStyle =
            FlatStyle::Flat;

        solveButton_->FlatAppearance->BorderSize =
            0;

        solveButton_->Click +=
            gcnew EventHandler(
                this,
                &MainForm::OnSolveClick);

        clearButton_ = gcnew Button();

        clearButton_->Text = L"Очистить";

        clearButton_->Size =
            Drawing::Size(150, 38);

        clearButton_->Visible = false;
        clearButton_->BackColor =
            Color::FromArgb(37, 99, 235);
        clearButton_->ForeColor = Color::White;
        clearButton_->FlatStyle = FlatStyle::Flat;
        clearButton_->FlatAppearance->BorderSize = 0;

        clearButton_->Click +=
            gcnew EventHandler(
                this,
                &MainForm::OnClearClick);

        buttons->Controls->Add(
            solveButton_);

        buttons->Controls->Add(
            clearButton_);

        equationCard->Controls->Add(
            equationTitle, 0, 0);

        equationCard->Controls->Add(
            equationPanel_, 0, 1);

        equationCard->Controls->Add(
            buttons, 0, 2);

        TableLayoutPanel^ contentLayout =
            gcnew TableLayoutPanel();

        contentLayout->Dock =
            DockStyle::Fill;

        contentLayout->ColumnCount = 2;
        contentLayout->RowCount = 1;

        contentLayout->ColumnStyles->Add(
            gcnew ColumnStyle(
                SizeType::Percent,
                38.0f));

        contentLayout->ColumnStyles->Add(
            gcnew ColumnStyle(
                SizeType::Percent,
                62.0f));

        TableLayoutPanel^ resultCard =
            gcnew TableLayoutPanel();

        resultCard->Dock =
            DockStyle::Fill;

        resultCard->ColumnCount = 1;
        resultCard->RowCount = 2;
        resultCard->BackColor = Color::White;

        resultCard->Padding =
            System::Windows::Forms::Padding(18);

        resultCard->Margin =
            System::Windows::Forms::Padding(
                0, 0, 12, 0);

        resultCard->RowStyles->Add(
            gcnew RowStyle(
                SizeType::AutoSize));

        resultCard->RowStyles->Add(
            gcnew RowStyle(
                SizeType::Percent,
                100.0f));

        Label^ resultTitle =
            gcnew Label();

        resultTitle->Text = L"Результат";
        resultTitle->AutoSize = true;

        resultTitle->Font =
            gcnew Drawing::Font(
                L"Segoe UI",
                11.0f,
                FontStyle::Bold);

        resultBox_ =
            gcnew RichTextBox();

        resultBox_->Dock =
            DockStyle::Fill;

        resultBox_->ReadOnly = true;
        resultBox_->BorderStyle =
            BorderStyle::None;

        resultBox_->BackColor =
            Color::White;

        resultBox_->Font =
            gcnew Drawing::Font(
                L"Consolas",
                11.0f);

        resultBox_->Margin =
            System::Windows::Forms::Padding(
                0, 14, 0, 0);

        resultCard->Controls->Add(
            resultTitle, 0, 0);

        resultCard->Controls->Add(
            resultBox_, 0, 1);

        TableLayoutPanel^ graphCard =
            gcnew TableLayoutPanel();

        graphCard->Dock =
            DockStyle::Fill;

        graphCard->ColumnCount = 1;
        graphCard->RowCount = 2;
        graphCard->BackColor = Color::White;

        graphCard->Padding =
            System::Windows::Forms::Padding(18);

        graphCard->Margin =
            System::Windows::Forms::Padding(0);

        graphCard->RowStyles->Add(
            gcnew RowStyle(
                SizeType::AutoSize));

        graphCard->RowStyles->Add(
            gcnew RowStyle(
                SizeType::Percent,
                100.0f));

        Label^ graphTitle =
            gcnew Label();

        graphTitle->Text =
            L"График функции";

        graphTitle->AutoSize = true;

        graphTitle->Font =
            gcnew Drawing::Font(
                L"Segoe UI",
                11.0f,
                FontStyle::Bold);

        graphTitle->Margin =
            System::Windows::Forms::Padding(0, 6, 0, 0);

        graphPanel_ =
            gcnew GraphPanel();

        graphPanel_->Dock =
            DockStyle::Fill;

        graphPanel_->Margin =
            System::Windows::Forms::Padding(
                0, 14, 0, 0);

        resetViewButton_ =
            gcnew Button();

        resetViewButton_->Text =
            L"Сбросить вид";

        resetViewButton_->AutoSize = true;

        resetViewButton_->Click +=
            gcnew EventHandler(
                this,
                &MainForm::OnResetViewClick);

        TableLayoutPanel^ graphHeader =
            gcnew TableLayoutPanel();

        graphHeader->AutoSize = true;
        graphHeader->Dock = DockStyle::Fill;
        graphHeader->ColumnCount = 2;
        graphHeader->ColumnStyles->Add(
            gcnew ColumnStyle(SizeType::Percent, 100.0f));
        graphHeader->ColumnStyles->Add(
            gcnew ColumnStyle(SizeType::AutoSize));
        graphHeader->Controls->Add(graphTitle, 0, 0);
        graphHeader->Controls->Add(resetViewButton_, 1, 0);

        graphCard->Controls->Add(
            graphHeader, 0, 0);

        graphCard->Controls->Add(
            graphPanel_, 0, 1);

        contentLayout->Controls->Add(
            resultCard, 0, 0);

        contentLayout->Controls->Add(
            graphCard, 1, 0);

        mainLayout->Controls->Add(
            title, 0, 0);

        mainLayout->Controls->Add(
            degreeCard, 0, 1);

        mainLayout->Controls->Add(
            equationCard, 0, 2);

        mainLayout->Controls->Add(
            contentLayout, 0, 3);

        Controls->Add(mainLayout);
    }
}

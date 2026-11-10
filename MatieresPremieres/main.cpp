#include "mainwindow.h"
#include <QApplication>

// Thème marine + crème (feuille de style intégrée, aucun fichier externe)
static const char *kStyle = R"QSS(
* { font-family: "Segoe UI", "Noto Sans", "DejaVu Sans", sans-serif; font-size: 9pt; color: #1d2b36; }
QMainWindow, QWidget#central, QWidget#page, QWidget#scrollInner { background: #f3ecda; }
QScrollArea { background: transparent; border: 0; }
QToolTip { background: #fffdf6; color: #1d2b36; border: 1px solid #1f3b4d; }
QFrame#sidebar { background: #1f3b4d; border: 0; }
QLabel#brand { color: white; background: transparent; font-size: 13pt; font-weight: 600; padding: 0 16px 22px 16px; }
QFrame#sidebar QPushButton { background: transparent; color: white; border: 0; border-radius: 4px;
    text-align: left; padding: 8px 8px; margin: 2px 8px; font-size: 9.5pt; }
QFrame#sidebar QPushButton:hover { background: rgba(255,255,255,0.14); }
QFrame#sidebar QPushButton:checked { background: #8c1c2e; }
QLabel#h1 { font-size: 17pt; font-weight: 700; color: #1b2e3b; }
QLabel#formTitle { font-size: 13pt; font-weight: 700; }
QLabel#panelTitle { font-size: 10.5pt; font-weight: 700; }
QLabel#hint, QLabel#muted { color: #5d6b75; font-size: 8.5pt; }
QLabel#muted { font-style: italic; }
QLabel#fieldLabel { font-weight: 700; }
QLabel#err { color: #b3261e; font-size: 8pt; }
QLabel#note { background: #fbf1cd; border: 1px solid #b9a45a; padding: 5px 10px; }
QLabel#mvInfo { background: #fffdf6; border: 1px solid #d6cbab; padding: 6px 8px; }
QLabel#mvInfo[invalid="true"] { color: #b3261e; border-color: #b3261e; }
QFrame#panel { background: #f3ecda; border: 1px solid #d6cbab; }
QFrame#panel QLabel { background: transparent; }
QFrame#kpi { background: #fffdf6; border: 1px solid #1f3b4d; }
QLabel#kpiHead { background: #1f3b4d; color: white; padding: 4px 8px; font-weight: 600; }
QLabel#kpiValue { font-size: 16pt; font-weight: 700; padding: 6px 8px 0 8px; background: transparent; }
QLabel#kpiSub { color: #5d6b75; padding: 2px 8px 6px 8px; background: transparent; }
QPushButton { background: #1f3b4d; color: white; border: 0; border-radius: 3px; padding: 5px 14px; min-height: 16px; }
QPushButton:hover { background: #2c5069; }
QPushButton:pressed { background: #16303f; }
QPushButton[variant="red"] { background: #8c1c2e; }
QPushButton[variant="red"]:hover { background: #a82a3e; }
QPushButton[variant="light"] { background: #e8e0c6; color: #1d2b36; border: 1px solid #d6cbab; }
QPushButton[variant="light"]:hover { background: #ded4b3; }
QPushButton:focus { border: 1px solid #f0b429; }
QPushButton#banner { background: #f9e3e1; color: #8a1f17; border: 1px solid #c0392b; border-radius: 2px;
    text-align: left; padding: 4px 10px; }
QPushButton#banner:hover { background: #f5d3d0; }
QLineEdit, QComboBox, QDateEdit, QSpinBox, QDoubleSpinBox, QPlainTextEdit {
    background: white; border: 1px solid #8d99a3; border-radius: 2px; padding: 2px 6px; min-height: 18px;
    selection-background-color: #1f3b4d; selection-color: white; }
QLineEdit:focus, QComboBox:focus, QDateEdit:focus, QSpinBox:focus, QPlainTextEdit:focus { border: 1px solid #2c5069; }
QLineEdit[readOnly="true"] { background: #ece6d1; }
QLineEdit[invalid="true"], QComboBox[invalid="true"], QDateEdit[invalid="true"], QPlainTextEdit[invalid="true"] {
    border: 1px solid #b3261e; background: #fff3f1; }
QComboBox QAbstractItemView { background: white; selection-background-color: #cddae4; selection-color: #1d2b36; }
QRadioButton { background: transparent; spacing: 6px; }
QRadioButton::indicator { width: 12px; height: 12px; border-radius: 7px; border: 1px solid #5d6b75; background: white; }
QRadioButton::indicator:checked { border-color: #1f3b4d; background: qradialgradient(cx:0.5, cy:0.5, radius:0.5, fx:0.5, fy:0.5, stop:0 #8c1c2e, stop:0.55 #8c1c2e, stop:0.65 white, stop:1 white); }
QComboBox::drop-down, QDateEdit::drop-down { border: 0; width: 20px; subcontrol-origin: padding; subcontrol-position: center right; }
QTableView { font-size: 8.5pt; background: #fffdf6; border: 1px solid #1f3b4d; gridline-color: #e6dec5;
    selection-background-color: #cddae4; selection-color: #1d2b36; }
QTableView::item { padding: 2px 4px; border-bottom: 1px solid #e6dec5; }
QTableView::item:hover { background: #f2ebd3; }
QTableView::item:selected { background: #cddae4; }
QHeaderView { background: #1f3b4d; }
QHeaderView::section { font-size: 8.5pt; background: #1f3b4d; color: white; padding: 4px 6px; border: 0;
    border-right: 1px solid #35566e; font-weight: 600; }
QTableCornerButton::section { background: #1f3b4d; border: 0; }
QScrollBar:vertical { background: #ece6d1; width: 12px; margin: 0; }
QScrollBar::handle:vertical { background: #b9ad88; min-height: 24px; border-radius: 3px; margin: 2px; }
QScrollBar:horizontal { background: #ece6d1; height: 12px; margin: 0; }
QScrollBar::handle:horizontal { background: #b9ad88; min-width: 24px; border-radius: 3px; margin: 2px; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
QLabel#toast { color: white; padding: 9px 14px; border-radius: 3px; }
QLabel#toast[type="ok"] { background: #2e7d32; }
QLabel#toast[type="err"] { background: #b3261e; }
QLabel#toast[type="warn"] { background: #b86e00; }
QLabel#toast[type="info"] { background: #1f3b4d; }
QMessageBox, QDialog { background: #f3ecda; }
QMessageBox QLabel, QDialog QLabel { background: transparent; }
QTabWidget#sections::pane { border: 0; background: #f3ecda; }
QTabWidget#sections QTabBar { background: #f3ecda; }
QTabBar::tab { background: transparent; color: #1f3b4d; padding: 9px 16px; margin-right: 2px;
               border: 0; border-bottom: 3px solid transparent; font-weight: 600; }
QTabBar::tab:hover { background: #e9e0c4; }
QTabBar::tab:selected { color: #8c1c2e; border-bottom: 3px solid #8c1c2e; }
)QSS";

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("GestionMatieres");
    app.setApplicationName("GestionMatieres");
    app.setStyle("Fusion");
    app.setStyleSheet(QString::fromUtf8(kStyle));

    // GMP_DB permet de choisir un autre fichier de base (utile pour les tests).
    MainWindow w(qEnvironmentVariable("GMP_DB"));
    w.showMaximized();
    return app.exec();
}

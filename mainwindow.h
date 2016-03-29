#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <vector>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionOpen_triggered();

    void on_actionTest_triggered();

    void on_actionSave_triggered();

    void on_horizontalSlider_start_valueChanged(int value);

    void on_horizontalSlider_end_valueChanged(int value);

    void on_doubleSpinBox_start_valueChanged(double arg1);

    void on_doubleSpinBox_end_valueChanged(double arg1);

    void on_pushButton_test_clicked();

    void on_pushButton_apply_clicked();

    void on_actionUndo_triggered();

    void on_pushButton_play_track_clicked();

    void on_actionRedo_triggered();

    void on_horizontalSlider_speed_valueChanged(int value);

    void on_horizontalSlider_ringmod_freq_valueChanged(int value);

    void on_horizontalSlider_ringmod_amp_valueChanged(int value);

    void on_horizontalSlider_amplify_valueChanged(int value);

    void on_radioButton_fade_in_clicked(bool checked);

    void on_radioButton_fade_out_clicked(bool checked);

    void on_radioButton_chop_in_clicked(bool checked);

    void on_radioButton_chop_out_clicked(bool checked);

    void on_verticalSlider_fade_tc_valueChanged(int value);

    void on_checkBox_mute_clicked(bool checked);

    void on_checkBox_reverse_clicked(bool checked);

    void on_spinBox_repeat_valueChanged(int value);

    void on_checkBox_repeat_clicked(bool checked);

    void on_tabWidget_currentChanged(int index);

private:
    void update_interval_selection_ranges();

    void update_stack_actions_availability();

    void apply_current_tab_mods();

    void changed_something();


    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H

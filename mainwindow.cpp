// Copyright CS 312, free open source
/****************************************************************
*
* Guts
* An audio modulation utility
* Ju Yun Kim kimj4
* Claire Tagoe tagoec
* 03/13/2016
*
***************************************************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QSound>
#include <QFile>

#include <iostream>
#include <string>
#include <cmath>

#include "sndfile.h"
#include "sndfile.hh"

#include <vector>

// ****************************** GLOBALS ****************************
const int kBUF_SZ = 1024;
float buffer[kBUF_SZ];
// name of original file
QString in_wav_name = "";

// temp file
QString out_wav_name = "";

// name of wav file containing only the interval
QString interval_wav_name = "";

// name of wav file containing everything, possibly with changes
QString temp_wav_name = "";

// special wav file for when chop is used
QString chopped_wav_name = "";

// contains samples from out_wav_name
std::vector<float> wav_vec;

// should be identical to the wav_vec but changes are made to this
std::vector<float> temp_wav_vec;

// a subset of temp_wav_vec on the selected interval
std::vector<float> interval_wav_vec;

bool changed = false;
double start_time;
double end_time;
int start_index = 0;
int end_index = 0;
int audio_sz = 0;
// determines what our tmp_wav_vec is in the stack
int current_track_index = 0;
// our stack
std::vector<std::vector<float>> audio_stack;

bool tested = false;

// generate sine wave constants
const int SR = 44100;  // CD sample rate
const float T = 1.0 / SR;

// ring modulation constants
float modulator_wave_amplitude = 0;
int modulator_wave_freq = 0;

// change speed constants
float playback_rate = 1.0;

// fade constants
float time_constant = 5;
bool fade_in_checked = true;
bool fade_out_checked = false;

// amplify constants
float amplitude_multiplier = 1;

// chop constants
bool chop_in_checked = true;
bool chop_out_checked = false;
std::vector<float> chopped_wav_vec;
bool chopped = false;

// mute constants
bool mute_checked = false;

// repeat constants
int number_of_repeats = 1;
bool repeat_checked;

// reverse constants
bool reverse_checked;
QString path;
// ****************************** end GLOBALS ****************************

// ******************************forward declarations*********************
void generate_sin_wave(float amp, float freq);
void ring_modulation(std::vector<float> sinevec);
void change_speed_call();
void fade_out();
void fade_in();
void amplify();
void chop_in();
void chop_out();
void mute();
void frequency_modulation();
void reverse();
void repeat();
// ******************************end forward declarations*****************

//*************************** Core Functions *****************************

// starts creating interval vec at the nearest sample number crossing the zero
// line
void make_interval_vec() {
  start_index = start_time * SR;
  end_index = end_time * SR;
  while (((temp_wav_vec.at(start_index) != 0.0) ||
          (temp_wav_vec.at(start_index) != -0.0))) {
    if (start_index == 0) break;
    start_index--;
  }
  while ((temp_wav_vec.at(end_index) != 0.0000) ||
         (temp_wav_vec.at(end_index) != -0.0000)) {
    if (end_index == static_cast<int>(temp_wav_vec.size() - 1)) break;
    end_index++;
  }
  for (int index = start_index; index <= end_index; ++index) {
    interval_wav_vec.push_back(temp_wav_vec.at(index));
  }
}

// called when apply button is clicked and changed == true
void apply_changes() {
  // if current_track_index is less than the size of the stack - 1, it means
  // that the user clicked apply after a series of undos
  // if this happens, erase the vectors in the front before applying.
  if (current_track_index < static_cast<int>((audio_stack.size() - 1))) {
    for (int i = current_track_index; i < static_cast<int>(audio_stack.size());
         ++i) {
      audio_stack.erase(audio_stack.begin() + i);
    }
  }

  // special case for chop
  if (chopped) {
    temp_wav_vec = chopped_wav_vec;
  } else {
    // insert the interval vector into the correct position in temp_wav_vec
    if (!interval_wav_vec.empty()) {
      // delete elements of the temp_wav_vec from start_index to end_index
      temp_wav_vec.erase(temp_wav_vec.begin() + start_index,
                         temp_wav_vec.begin() + end_index);
      // insert elements of the interval_wav_vec at start_index - 1
      temp_wav_vec.insert(temp_wav_vec.begin() + start_index - 1,
                          interval_wav_vec.begin(), interval_wav_vec.end());
    }
  }
  audio_stack.push_back(temp_wav_vec);

  // users are limited to 10 vectors. if there are more, remove the vector from
  // the very bottom of the stack.
  if (audio_stack.size() >= 11) {
    audio_stack.erase(audio_stack.begin());
  }

  // after applying changes, the current_track_index should point to the top of
  // the stack.
  current_track_index = static_cast<int>(audio_stack.size() - 1);
}

// goes back to the temp_wav_vec from the last time the apply button was pressed
void undo() {
  current_track_index--;
  temp_wav_vec = audio_stack.at(current_track_index);
}

// if the undo was ever called, revert back to the temp_wav_vec from before
void redo() {
  current_track_index++;
  temp_wav_vec = audio_stack.at(current_track_index);
}

void show_message(QString qs) {
  QMessageBox msgBox;
  msgBox.setText(qs);
  msgBox.exec();
}

int make_temp_wav_vec(QString qs) {
  SF_INFO sfinfo;
  SNDFILE* infile;
  sf_count_t count;
  std::string str(qs.toStdString());
  char* in_fname = const_cast<char*>(str.c_str());
  if ((infile = sf_open(in_fname, SFM_READ, &sfinfo)) == NULL) {
    sf_close(infile);
  }

  // get the size
  sf_count_t sz = sfinfo.frames;  // sample count
  int actual_sz = 0;
  int vec_sz = 0;
  while ((count = sf_read_float(infile, buffer, kBUF_SZ)) > 0) {
    actual_sz += count;
    std::vector<float> vec;
    for (int i = 0; i < 1024; i++) {
      vec_sz++;
      if (vec_sz > actual_sz) break;
      vec.push_back(buffer[i]);
    }
    std::copy(vec.begin(), vec.end(), std::back_inserter(wav_vec));
  }
  audio_sz = sz;
  sf_close(infile);
  temp_wav_vec.clear();
  std::copy(wav_vec.begin(), wav_vec.end(), std::back_inserter(temp_wav_vec));
  // coppy into a temp_wav_vec from where changes will be made
  return 0;
}

// makes a .wav file from either interval_wav_vec or temp_wav_vec
// depending on selection
// more might be added later
void make_file(int selection) {
  const int format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
  const int channels = 1;  // mono wave file
  char* out_fname;

  // selection == 0 creates temp_wav_file
  // selection == 1 creates interval_wav_file
  // selection == 2 creates chopped_wav_file
  if (selection == 0) {
    out_fname = const_cast<char*>(
        temp_wav_name.toStdString().c_str());  // ******* get path
    SndfileHandle outfile(out_fname, SFM_WRITE, format, channels, SR);
    if (not outfile) return;
    outfile.write(&temp_wav_vec[0], temp_wav_vec.size());
  } else if (selection == 1) {
    out_fname = const_cast<char*>(
        interval_wav_name.toStdString().c_str());  // ******* get path
    SndfileHandle outfile(out_fname, SFM_WRITE, format, channels, SR);
    if (not outfile) return;
    outfile.write(&interval_wav_vec[0], interval_wav_vec.size());
  } else if (selection == 2) {
    out_fname = const_cast<char*>(
        chopped_wav_name.toStdString().c_str());  // ******* get path
    SndfileHandle outfile(out_fname, SFM_WRITE, format, channels, SR);
    if (not outfile) return;
    outfile.write(&chopped_wav_vec[0], chopped_wav_vec.size());
  }
}

// plays a .wav file corresponding to either interval_wav_vec or temp_wav_vec
// prerequisite: the .wav files were created.
void play_wav(int selection) {
  // selection == 0 plays the whole track (temp_wav_vec)
  // selection == 1 plays the interval track (interval_wav_vec)
  // selection == 2 plays the chopped track (chopped_wav_vec)
  SF_INFO sfinfo;
  SNDFILE* infile;
  if (selection == 0) {
    std::string str(temp_wav_name.toStdString());
    char* in_fname = const_cast<char*>(str.c_str());
    if ((infile = sf_open(in_fname, SFM_READ, &sfinfo)) == NULL) {
      sf_close(infile);
    }
    QSound::play(temp_wav_name);
    sf_close(infile);

  } else if (selection == 1) {
    std::string str(interval_wav_name.toStdString());
    char* in_fname = const_cast<char*>(str.c_str());
    if ((infile = sf_open(in_fname, SFM_READ, &sfinfo)) == NULL) {
      sf_close(infile);
    }
    QSound::play(interval_wav_name);
    sf_close(infile);
  } else if (selection == 2) {
    std::string str(chopped_wav_name.toStdString());
    char* in_fname = const_cast<char*>(str.c_str());
    if ((infile = sf_open(in_fname, SFM_READ, &sfinfo)) == NULL) {
      sf_close(infile);
    }
    QSound::play(chopped_wav_name);
    sf_close(infile);
  }
}




/*************************** utility functions **************************/
void initalize_file_names() {
  out_wav_name = in_wav_name;
  out_wav_name.chop(4);
  out_wav_name = out_wav_name + "_tmp.wav";

  interval_wav_name = in_wav_name;
  interval_wav_name.chop(4);
  interval_wav_name = interval_wav_name + "_tmp_interval.wav";

  temp_wav_name = in_wav_name;
  temp_wav_name.chop(4);
  temp_wav_name = temp_wav_name + "_tmp_whole.wav";

  chopped_wav_name = in_wav_name;
  chopped_wav_name.chop(4);
  chopped_wav_name = chopped_wav_name + "_tmp_chopped.wav";
}

/*************************** end utility functions **********************/

/*************************** End Core Functions *************************/

/*************************** MainWindow::********************************/

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
}

MainWindow::~MainWindow() {
  delete ui;
  QFile::remove(out_wav_name);
  QFile::remove(interval_wav_name);
  QFile::remove(temp_wav_name);
  QFile::remove(chopped_wav_name);
}

/************************ Audio Playback UI ********************/
void MainWindow::on_actionOpen_triggered() {
  QString filter = "File type (*.wav)";
  in_wav_name =
      QFileDialog::getOpenFileName(this, "Open a wav file", path, filter);
  if (in_wav_name == "") {
    // this is if user selects cancel from the open window
    // then abort and just go back to whatever they were doing
    return;
  } else {
    // if successfully opened a new file, delete the temps of the old files
    QFile::remove(out_wav_name);
    QFile::remove(interval_wav_name);
    QFile::remove(temp_wav_name);
    QFile::remove(chopped_wav_name);

    // then clear all the vectors
    audio_stack.clear();
    temp_wav_vec.clear();
    interval_wav_vec.clear();
    wav_vec.clear();

    // then initialize new names with the new in_wav_name
    initalize_file_names();

    QFile::copy(in_wav_name, out_wav_name);

    // then initialize all conditions and variables to start usage
    std::string str = out_wav_name.toStdString();
    char* s = const_cast<char*>(str.c_str());
    make_temp_wav_vec(s);
    update_interval_selection_ranges();
  }
}

void MainWindow::on_actionTest_triggered() {
  tested = true;
  interval_wav_vec.clear();
  if (out_wav_name != "") {
    make_interval_vec();
    apply_current_tab_mods();
    if (chopped) {
      make_file(2);
      play_wav(2);
    } else {
      make_file(1);
      play_wav(1);
    }
  }
}

void MainWindow::on_actionSave_triggered() {
  if (in_wav_name == "") return;
  make_file(0);
  QString filter = "File type (*.wav)";
  QString name =
      QFileDialog::getSaveFileName(this, "Save file as", path, filter);
  QFile::copy(temp_wav_name, name);
}

void MainWindow::on_actionUndo_triggered() {
  undo();
  update_stack_actions_availability();
  update_interval_selection_ranges();
}

void MainWindow::on_actionRedo_triggered() {
  redo();
  update_stack_actions_availability();
  update_interval_selection_ranges();
}

void MainWindow::on_pushButton_test_clicked() {
  MainWindow::on_actionTest_triggered();
}

void MainWindow::on_pushButton_apply_clicked() {
  if (!tested) {
    interval_wav_vec.clear();
    make_interval_vec();
    apply_current_tab_mods();
    if (chopped) {
      make_file(2);
    } else {
      make_file(1);
    }
  }

  // if stack is not empty and there was a change, apply change
  if (audio_stack.size() > 0) {
    if (changed) {
      apply_changes();
      changed = false;
      tested = false;
    }
    // if stack is empty, allow apply change even if there was no change.
  } else {
    apply_changes();
    changed = false;
    tested = false;
  }
  update_stack_actions_availability();
  update_interval_selection_ranges();
}

void MainWindow::on_pushButton_play_track_clicked() {
  if (in_wav_name != "") {
    make_interval_vec();
    make_file(0);
    play_wav(0);
  }
}
/************************ end Audio Playback UI ********************/

/************************ mainwindow utility functions *************/
// updates the scrollable time ranges for slider and boxes
// called at open, undo, redo, apply
void MainWindow::update_interval_selection_ranges() {
  float float_SR = 44100.0;
  ui->horizontalSlider_start->setRange(0, temp_wav_vec.size() * 100 / float_SR);
  ui->horizontalSlider_end->setRange(0, temp_wav_vec.size() * 100 / float_SR);
  ui->doubleSpinBox_start->setRange(0, temp_wav_vec.size() / float_SR);
  ui->doubleSpinBox_end->setRange(0, temp_wav_vec.size() / float_SR);
}

// disables or enables the undo or redo actions depending on which vector
//   from audio_stack is the current temp_wav_vec
// called at undo, redo, apply
void MainWindow::update_stack_actions_availability() {
  if (current_track_index == static_cast<int>(audio_stack.size() - 1)) {
    ui->actionRedo->setEnabled(false);
  } else {
    ui->actionRedo->setEnabled(true);
  }
  if (current_track_index > 0) {
    ui->actionUndo->setEnabled(true);
  } else {
    ui->actionUndo->setEnabled(false);
  }
}

// applies the modifications that may have been made in the currently open tab
// called in test
// may be called in apply if test was not called previously
void MainWindow::apply_current_tab_mods() {
  int current_tab = ui->tabWidget->currentIndex();
  switch (current_tab) {
    case 0:
      break;
    case 1:
      if (fade_in_checked) {
        fade_in();
      } else if (fade_out_checked) {
        fade_out();
      }
      break;
    case 2:
      if (repeat_checked) repeat();
      break;
    case 3:
      if (reverse_checked) reverse();
      break;
    case 4:
      amplify();
      break;
    case 5:
      change_speed_call();
      break;
    case 6:
      generate_sin_wave(modulator_wave_amplitude, modulator_wave_freq);
      break;
    case 7:
      if (chop_in_checked) {
        chop_in();
      } else if (chop_out_checked) {
        chop_out();
      }
      break;
    case 8:
      if (mute_checked) {
        mute();
      }
      break;
    default:
      break;
  }
}

void MainWindow::changed_something() {
  changed = true;
  tested = false;
}

// if the tab is changed, clicking apply changes will
// save the current vector
void MainWindow::on_tabWidget_currentChanged(int index) { changed_something(); }
/************************ end mainwindow utility functions *********/

/************************ Interval Selection UI ********************/

void MainWindow::on_horizontalSlider_start_valueChanged(int value) {
  if (value >= ui->horizontalSlider_end->value()) {
    ui->horizontalSlider_start->setValue(ui->horizontalSlider_end->value() - 1);
    ui->doubleSpinBox_start->setValue(ui->doubleSpinBox_end->value() - .01);
  } else {
    ui->doubleSpinBox_start->setValue(value / 100.0);
  }
  start_time = ui->doubleSpinBox_start->value();
}

void MainWindow::on_horizontalSlider_end_valueChanged(int value) {
  if (value <= ui->horizontalSlider_start->value()) {
    ui->horizontalSlider_end->setValue(ui->horizontalSlider_start->value() + 1);
    ui->doubleSpinBox_end->setValue(ui->doubleSpinBox_start->value() + .01);
  } else {
    ui->doubleSpinBox_end->setValue(value / 100.0);
  }
  end_time = ui->doubleSpinBox_end->value();
}

void MainWindow::on_doubleSpinBox_start_valueChanged(double arg1) {
  if (arg1 >= (ui->doubleSpinBox_end->value())) {
    ui->horizontalSlider_start->setValue(ui->horizontalSlider_end->value() - 1);
    ui->doubleSpinBox_start->setValue(ui->doubleSpinBox_end->value() - .01);
  } else {
    ui->horizontalSlider_start->setValue(arg1 * 100.0);
  }
  start_time = ui->doubleSpinBox_start->value();
}

void MainWindow::on_doubleSpinBox_end_valueChanged(double arg1) {
  if (arg1 <= (ui->doubleSpinBox_start->value())) {
    ui->horizontalSlider_end->setValue(ui->horizontalSlider_start->value() + 1);
    ui->doubleSpinBox_end->setValue(ui->doubleSpinBox_start->value() + .01);
  } else {
    ui->horizontalSlider_end->setValue(arg1 * 100.0);
  }
  end_time = ui->doubleSpinBox_end->value();
}
/************************ end Interval Selection UI ********************/

/************************ ring modulation ******************************/
void MainWindow::on_horizontalSlider_ringmod_freq_valueChanged(int value) {
  changed_something();
  modulator_wave_freq = value;
  ui->label_ringmod_freq->setNum(value);
}

void MainWindow::on_horizontalSlider_ringmod_amp_valueChanged(int value) {
  changed_something();
  modulator_wave_amplitude = value / 100.0;
  ui->label_ringmod_amp->setNum(value / 100.0);
}

void generate_sin_wave(float amp, float freq) {
  int kDUR =
      interval_wav_vec.size() - 1;  // create a sine wave with sample number
                                    // equal to that of interval_wav to be
                                    // edited.
  const float kTWOPI_T = 2 * M_PI * T;
  std::vector<float> sinevec;

  for (int t = 0; t < kDUR; ++t) {
    sinevec.push_back(amp * sin(kTWOPI_T * freq * t));
  }
  ring_modulation(sinevec);
}

void ring_modulation(std::vector<float> sinevec) {
  std::vector<float> temp_vec;

  std::transform(interval_wav_vec.begin(), interval_wav_vec.end(),
                 sinevec.begin(), std::back_inserter(temp_vec),
                 std::multiplies<float>());

  interval_wav_vec = temp_vec;
}
/************************end ring modulation ********************/

/************************change speed*****************************/
// change speed function from JE
std::vector<float> change_speed(const std::vector<float>& wav, float rate) {
  std::vector<float> vec;
  int L = wav.size();
  int numSamples = L / rate;
  float increment = rate;
  float previous_phase = 0;

  for (int outIndx = 0; outIndx < numSamples; ++outIndx) {
    float phase_index = std::fmod(previous_phase + increment, L);
    int leftIndx = std::floor(phase_index) + 1;
    if (leftIndx > L) leftIndx = leftIndx % L;

    int rightIndx = leftIndx + 1;
    if (rightIndx > L) rightIndx = rightIndx % L;

    float decimalPart = std::remainder(phase_index, 1.0);
    float isamp =
        wav[leftIndx] + decimalPart * (wav[rightIndx] - wav[leftIndx]);
    vec.push_back(isamp);

    previous_phase = phase_index;
  }
  return vec;
}

void MainWindow::on_horizontalSlider_speed_valueChanged(int value) {
  changed_something();
  playback_rate = value / 100.0;
  ui->label_speed->setNum(value);
}

void change_speed_call() {
  std::vector<float> temp_vec = change_speed(interval_wav_vec, playback_rate);
  interval_wav_vec = temp_vec;
}
/************************end change speed********************/

/************************Fade *******************************/
std::vector<float> exp_decay_envelope_fade_out(float tc, int total_samples) {
  std::vector<float> exp_decay_vec;
  float temp_total = total_samples;
  for (int n = 0; n < temp_total; ++n) {
    float pusher = exp((-tc * n) / temp_total);
    exp_decay_vec.push_back(pusher);
  }
  return exp_decay_vec;
}

std::vector<float> exp_decay_envelope_fade_in(float tc, int total_samples) {
  std::vector<float> exp_decay_vec;
  float temp_total = total_samples;
  for (int n = 0; n < temp_total; ++n) {
    float pusher = -exp((-tc * n) / temp_total) + 1;
    exp_decay_vec.push_back(pusher);
  }
  return exp_decay_vec;
}

void MainWindow::on_verticalSlider_fade_tc_valueChanged(int value) {
  changed_something();
  time_constant = value / 10.0;
  ui->label_fade_tc_value->setNum(value / 10.0);
}

void MainWindow::on_radioButton_fade_in_clicked(bool checked) {
  if (checked) {
    changed_something();
    fade_out_checked = false;
    fade_in_checked = true;
    ui->radioButton_fade_out->setChecked(false);
  }
}

void MainWindow::on_radioButton_fade_out_clicked(bool checked) {
  if (checked) {
    changed_something();
    fade_out_checked = true;
    fade_in_checked = false;
    ui->radioButton_fade_in->setChecked(false);
  }
}

void fade_out() {
  int number_of_samples = interval_wav_vec.size();
  std::vector<float> decay_vec =
      exp_decay_envelope_fade_out(time_constant, number_of_samples);
  std::vector<float> temp_vec;
  std::transform(decay_vec.begin(), decay_vec.end(), interval_wav_vec.begin(),
                 std::back_inserter(temp_vec), std::multiplies<float>());
  interval_wav_vec = temp_vec;
}

void fade_in() {
  int number_of_samples = interval_wav_vec.size();
  std::vector<float> decay_vec =
      exp_decay_envelope_fade_in(time_constant, number_of_samples);
  std::vector<float> temp_vec;
  std::transform(decay_vec.begin(), decay_vec.end(), interval_wav_vec.begin(),
                 std::back_inserter(temp_vec), std::multiplies<float>());
  interval_wav_vec = temp_vec;
}
/************************end Fade *******************************/

/************************Amplify *******************************/
void MainWindow::on_horizontalSlider_amplify_valueChanged(int value) {
  changed_something();
  amplitude_multiplier = (value / 100.0);
  ui->label_amplify->setNum(value);
}

void amplify() {
  std::transform(interval_wav_vec.begin(), interval_wav_vec.end(),
                 interval_wav_vec.begin(),
                 std::bind1st(std::multiplies<float>(), amplitude_multiplier));
}
/************************end Amplify *******************************/

/************************ Chop *************************************/
void MainWindow::on_radioButton_chop_in_clicked(bool checked) {
  if (checked) {
    changed_something();
    chop_in_checked = true;
    chop_out_checked = false;
    ui->radioButton_chop_out->setChecked(false);
  }
}

void MainWindow::on_radioButton_chop_out_clicked(bool checked) {
  if (checked) {
    changed_something();
    chop_in_checked = false;
    chop_out_checked = true;
    ui->radioButton_chop_in->setChecked(false);
  }
}

void chop_in() {
  chopped = true;
  chopped_wav_vec = interval_wav_vec;
}

void chop_out() {
  chopped_wav_vec = temp_wav_vec;
  chopped_wav_vec.erase(chopped_wav_vec.begin() + start_index,
                        chopped_wav_vec.begin() + end_index);
}

/************************ end Chop **********************************/

/************************ Mute **************************************/
void MainWindow::on_checkBox_mute_clicked(bool checked) {
  changed_something();
  if (checked) {
    mute_checked = true;
  } else {
    mute_checked = false;
  }
}

void mute() {
  std::transform(interval_wav_vec.begin(), interval_wav_vec.end(),
                 interval_wav_vec.begin(),
                 std::bind1st(std::multiplies<float>(), 0));
}
/************************ end Mute **********************************/

/************************ repeat **********************************/

void MainWindow::on_spinBox_repeat_valueChanged(int value) {
  changed_something();
  number_of_repeats = value;
}

void MainWindow::on_checkBox_repeat_clicked(bool checked) {
  changed_something();
  if (checked)
    repeat_checked = true;
  else
    repeat_checked = false;
}

void repeat() {
  std::vector<float> temp_vec;
  for (int i = 0; i < number_of_repeats; ++i) {
    for (auto sample : interval_wav_vec) {
      temp_vec.push_back(sample);
    }
  }
  interval_wav_vec = temp_vec;
}

/************************ repeat **********************************/

/************************ reverse **********************************/
void MainWindow::on_checkBox_reverse_clicked(bool checked) {
  changed_something();
  if (checked) {
    reverse_checked = true;
  } else {
    reverse_checked = false;
  }
}

void reverse() {
  std::vector<float> temp_vec;
  for (int i = interval_wav_vec.size() - 1; i > -1; --i) {
    temp_vec.push_back(interval_wav_vec.at(i));
  }
  interval_wav_vec = temp_vec;
}
/************************ reverse **********************************/

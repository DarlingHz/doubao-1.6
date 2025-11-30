#include "submission.h"

int Submission::GetId() const {
    return id_;
}

void Submission::SetId(int id) {
    id_ = id;
}

int Submission::GetUserId() const {
    return user_id_;
}

void Submission::SetUserId(int user_id) {
    user_id_ = user_id;
}

int Submission::GetProblemId() const {
    return problem_id_;
}

void Submission::SetProblemId(int problem_id) {
    problem_id_ = problem_id;
}

SubmissionStatus Submission::GetStatus() const {
    return status_;
}

void Submission::SetStatus(SubmissionStatus status) {
    status_ = status;
}

int Submission::GetTimeSpentSeconds() const {
    return time_spent_seconds_;
}

void Submission::SetTimeSpentSeconds(int time_spent_seconds) {
    time_spent_seconds_ = time_spent_seconds;
}

std::string Submission::GetNote() const {
    return note_;
}

void Submission::SetNote(const std::string& note) {
    note_ = note;
}

std::chrono::system_clock::time_point Submission::GetCreatedAt() const {
    return created_at_;
}

void Submission::SetCreatedAt(const std::chrono::system_clock::time_point& created_at) {
    created_at_ = created_at;
}

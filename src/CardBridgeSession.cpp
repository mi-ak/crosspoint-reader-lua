#include "CardBridgeSession.h"

#include <Arduino.h>

#include "CardBridgeTokens.h"

CardBridgeSession CardBridgeSession::instance_;

CardBridgeSession& CardBridgeSession::getInstance() {
  return instance_;
}

std::string CardBridgeSession::beginPairing() {
  pairingToken_ = CardBridgeTokens::generate();
  pairingExpiry_ = millis() + PAIRING_TTL_MS;

  // Reset any active session
  sessionToken_.clear();
  nextToken_.clear();
  prevToken_.clear();
  sessionExpiry_ = 0;
  prevTokenExpiry_ = 0;
  state_ = State::WAITING_PAIR;

  return pairingToken_;
}

std::string CardBridgeSession::claimSession(const std::string& pairingToken) {
  if (pairingToken_.empty() || pairingToken != pairingToken_) {
    return {};
  }
  if (millis() > pairingExpiry_) {
    pairingToken_.clear();
    return {};
  }

  pairingToken_.clear();
  sessionToken_ = CardBridgeTokens::generate();
  nextToken_ = CardBridgeTokens::generate();
  prevToken_.clear();
  prevTokenExpiry_ = 0;
  sessionExpiry_ = millis() + SESSION_TTL_MS;
  state_ = State::ACTIVE;

  return sessionToken_;
}

bool CardBridgeSession::validate(const std::string& bearerToken) {
  if (state_ != State::ACTIVE) return false;
  if (millis() > sessionExpiry_) return false;
  if (!bearerToken.empty()) {
    if (bearerToken == sessionToken_) return true;
    if (bearerToken == nextToken_) return true;
    if (!prevToken_.empty() && bearerToken == prevToken_ && millis() <= prevTokenExpiry_) return true;
  }
  return false;
}

std::string CardBridgeSession::heartbeat(const std::string& bearerToken) {
  if (!validate(bearerToken)) return {};

  // Roll tokens
  prevToken_ = nextToken_;
  prevTokenExpiry_ = millis() + PREV_TOKEN_GRACE_MS;
  // Update current session token to the old next token when caller used sessionToken
  if (bearerToken == sessionToken_) {
    sessionToken_ = nextToken_;
  }
  nextToken_ = CardBridgeTokens::generate();
  sessionExpiry_ = millis() + SESSION_TTL_MS;

  return nextToken_;
}

void CardBridgeSession::closeSession(const std::string& bearerToken) {
  if (!validate(bearerToken)) return;

  state_ = State::EXPIRED;
  sessionToken_.clear();
  nextToken_.clear();
  prevToken_.clear();
  sessionExpiry_ = 0;
  prevTokenExpiry_ = 0;
}

CardBridgeSession::State CardBridgeSession::getState() const {
  return state_;
}

bool CardBridgeSession::isPairingActive() const {
  return state_ == State::WAITING_PAIR && !pairingToken_.empty() && millis() <= pairingExpiry_;
}

void CardBridgeSession::tick(unsigned long nowMs) {
  if (state_ == State::ACTIVE && nowMs > sessionExpiry_) {
    state_ = State::EXPIRED;
    sessionToken_.clear();
    nextToken_.clear();
  }
  if (!prevToken_.empty() && nowMs > prevTokenExpiry_) {
    prevToken_.clear();
  }
}

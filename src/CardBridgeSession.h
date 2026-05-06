#pragma once
#include <cstdint>
#include <string>

// Single session manager for X4 Card Bridge
// v1.0: only one session at a time
class CardBridgeSession {
 public:
  enum class State { WAITING_PAIR, ACTIVE, EXPIRED };

  static CardBridgeSession& getInstance();

  // Pairing
  // Returns a new pairing token (valid for PAIRING_TTL_MS)
  std::string beginPairing();
  // Validate pairing token and issue session token
  // Returns session_token on success, empty string on failure
  std::string claimSession(const std::string& pairingToken);

  // Session
  // Validate bearer token; returns true if valid (active and not expired)
  bool validate(const std::string& bearerToken);
  // Heartbeat: validate token, return next token (rolling); empty on failure
  std::string heartbeat(const std::string& bearerToken);
  // Close session
  void closeSession(const std::string& bearerToken);

  State getState() const;
  bool isPairingActive() const;

  // Called in loop() to handle TTL expiry
  void tick(unsigned long nowMs);

 private:
  CardBridgeSession() = default;
  static CardBridgeSession instance_;

  State state_ = State::WAITING_PAIR;
  std::string pairingToken_;
  unsigned long pairingExpiry_ = 0;

  std::string sessionToken_;
  std::string nextToken_;
  unsigned long prevTokenExpiry_ = 0;
  std::string prevToken_;
  unsigned long sessionExpiry_ = 0;

  static constexpr unsigned long PAIRING_TTL_MS = 5UL * 60 * 1000;   // 5 minutes
  static constexpr unsigned long SESSION_TTL_MS = 60UL * 1000;        // 60 seconds
  static constexpr unsigned long PREV_TOKEN_GRACE_MS = 15UL * 1000;   // 15 seconds grace
};

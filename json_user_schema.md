JSON Schema:
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "title": "Secure Lock Box User Database",
  "description": "Schema for user authentication data stored on SD card as JSON.",
  "type": "object",
  "properties": {
    "users": {
      "type": "array",
      "title": "List of registered users",
      "items": {
        "type": "object",
        "required": ["USERID", "PIN"],
        "properties": {
          "USERID": {
            "type": "integer",
            "description": "Unique system-wide user identifier."
          },
          "FINGERID": {
            "type": ["integer", "null"],
            "description": "User's fingerprint ID if registered."
          },
          "FACEID": {
            "type": ["integer", "null"],
            "description": "User's facial recognition ID if registered."
          },
          "PIN": {
            "type": "string",
            "pattern": "^[0-9]{4,8}$",
            "description": "User PIN code (4–8 digits)."
          },
          "USERNAME": {
            "type": "string",
            "maxLength": 32,
            "description": "User's display name."
          },
          "ADMIN": {
            "type": "boolean",
            "default": false,
            "description": "Administrative access flag."
          },
          "LAST_LOGON": {
            "type": ["string", "null"],
            "format": "date-time",
            "description": "ISO 8601 timestamp of last logon or null if never logged in."
          }
        },
        "additionalProperties": false
      }
    }
  },
  "required": ["users"],
  "additionalProperties": false
}
 
 
Here is an example JSON instance with a few users that conforms to the schema:
{
  "users": [
    {
      "USERID": 1,
      "FINGERID": 10,
      "FACEID": 5,
      "PIN": "1234",
      "USERNAME": "Alice",
      "ADMIN": true,
      "LAST_LOGON": "2026-01-18T13:00:00Z"
    },
    {
      "USERID": 2,
      "FINGERID": 11,
      "FACEID": null,
      "PIN": "7654",
      "USERNAME": "Bob",
      "ADMIN": false,
      "LAST_LOGON": "2026-01-17T22:15:30Z"
    },
    {
      "USERID": 3,
      "FINGERID": null,
      "FACEID": 8,
      "PIN": "5555",
      "USERNAME": "Guest User",
      "ADMIN": false,
      "LAST_LOGON": null
    }
  ]
}

•	null for FINGERID or FACEID indicates that biometric is not enrolled for that user, which keeps parsing simple on the ESP32-S3.
•	LAST_LOGON uses ISO 8601 UTC timestamps so you can parse or compare them easily if you later add features like inactivity lockout or audit logs.


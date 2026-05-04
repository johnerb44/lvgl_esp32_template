# PIN Authentication Flow Testing

## Feature: Secure lockbox PIN authentication

### Background:
    - Given the secure lockbox is powered on
	- And there exists a user in the user database with Name = "Alice"
	- And the user named "Alice" has a PIN of "4321"
	- And the user has successfully authenticated using face or fingerprint
    - And the system is on the PIN entry screen

### Scenario: 1 User enters valid PIN
    - Then PIN Entry screen dislays message "Welcome, Alice" 
	- Then PIN Entry screen displays numeric keypad
	- Then "View PIN" button is not selected
	- Then "    " is displayed in PIN field
	- When the user enters PIN "4321" on keypad
	- Then "----" is displayed in the PIN field
	- When user presses "Backspace" button one time
	- Then "--- " is displayed in the PIN field
	- When user enters "1" on the numeric keypad
	- Then PIN Entry screen displays "----" in the PIN field
	- When user presses "Clear" on the numeric keypad
	- Then "    " is displayed in the PIN field
	- When the user selects the "View PIN" button
	- Then "    " is displayed in the PIN field
	- When the user enters PIN "4321" on keypad
	- Then "4321" is displayed in the PIN field
	- When the user selects the "Submit" button
	- Then the system attempts to match entered PIN with user's json user database PIN field
	- And PINs match
    - Then the system displays "PIN Accepted" for 3 seconds
    - Then the system proceeds to the Home screen


### Scenario: 2 User enters invalid PIN
    - Then PIN Entry screen dislays message "Welcome, Alice" 
	- Then PIN Entry screen displays numeric keypad
	- Then "View PIN" button is not selected
	- Then "    " is displayed in PIN field
	- When the user enters PIN "5554" on keypad
	- Then "----" is displayed in the PIN field
	- When user presses "Backspace" button one time
	- Then "--- " is displayed in the PIN field
	- When user enters "4" on the numeric keypad
	- Then PIN Entry screen displays "----" in the PIN field
	- When user presses "Clear" on the numeric keypad
	- Then "    " is displayed in the PIN field
	- When the user selects the "View PIN" button
	- Then "    " is displayed in the PIN field
	- When the user enters PIN "5554" on keypad
	- Then "5554" is displayed in the PIN field
	- When the user presses the "Submit" button
	- Then the system attempts to match entered PIN with user's json user database PIN field
	- And PINs do not match
    - Then the system displays "Invalid PIN" for 5 seconds
	- When the user enters a valid PIN in less than 10 tries
	- Then the system attempts to match entered PIN with user's json user database PIN field
	- And PINs match
    - Then the system displays "PIN Accepted" for 3 seconds
    - Then the system proceeds to the Home screen
	- When the user enters an invalid PIN 10 times
    - Then the system stays on PIN Entry screen	
	- When user enters an invalid PIN ten times
	- Then the system does not allow the user to try authentication again for 60 seconds
	- Then system proceeds to the Main screen
	
	
	















## Feature: Secure lockbox PIN authentication

### Background:
    - Given the secure lockbox is powered on
	- And there exists a user in the user database with Name = "Alice"
	- And the user named "Alice" has a PIN of "4321"
	- And the user has successfully authenticated face or fingerprint
    - And the system is on the PIN entry screen

### Scenario: Secure lockbox PIN authentication and valid PIN
    - When the user enters PIN "4321"
	- And the user named "Alice" has a PIN of "4321"
    - And the user selects "Submit"
    - Then the system displays "PIN Accepted" for 3 seconds
    - Then the system proceeds to the home screen
	
	Home screen Scenarios
    - And the system displays "Welcome, Alice"
    - And the lock status is "[LOCKED]"
    - When the user selects "Unlock Secure Box"
    - Then the system displays "Box Unlocked"
     -And the lock status changes to "[UNLOCKED]"


Here are Gherkin-style rewrites for the tests marked **tested**, based on your `testing.md` file. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/a9c00232-12e7-4453-aada-d818f8bd42ea/testing.md?AWSAccessKeyId=ASIA2F3EMEYEYM47R5KJ&Signature=hqaiIHamn2xjO8imREg6oV8NimQ%3D&x-amz-security-token=IQoJb3JpZ2luX2VjEKv%2F%2F%2F%2F%2F%2F%2F%2F%2F%2FwEaCXVzLWVhc3QtMSJGMEQCIAeN0Un2%2FoNn0pntXjOvv0N18zF7h2wn05bTbLc26TCyAiBh4S0onJJ8s3B0MjCrWdW9Q66KxpaCQWX%2FYFpL%2FvamhyrzBAh0EAEaDDY5OTc1MzMwOTcwNSIMrEbTFrUSVFeSyIjrKtAECyaZMZ4K9xrt6aHtkgTYCX6WfKKS7OaEQk8Tnk790ozoLYd9ZLGGSCgyDivj28dYHE8qb1uU6HbcYcD6PavOMUxAiaK8pITADt%2BAJmeR6G6bPoPZqHIEeWtQJR6cBUTSF2teYAluXnQ4aW789F3LOKHMmdUSpAfspVuwObtm4SR%2FWTtQxgINFSe7D6CMzhBgHbEB2iAMY2w5LyLLCp%2BUHxxWQXGGU4tSGYzDgpva9kPRyJPUcjxq%2B%2BKJfFUmprSFc0BALTfwpUPLYc7OfV%2F37tcnpUDwSdU9Vac59QMvA4bbDD61ZBKyvp8YneF3ASs16DkIoNFbu%2Fcs1uUUM9Vm0nlC88EMABzFKwEC%2FqfqTSGsL0TXzx3VDi9k%2B6gSeWbEDGwH0GIm%2BThM78aNvSWSxL7L0MgVTUooW5olCRqIFpyN04qLQN%2FI%2Bxa%2FgwbdoVcfbvgNlJhg4Tho%2BJF258SKJSCU2%2BAPrLjabH5RaFBO0JgHE8cZA74PeIbiEao4pq1xDVfBiBl1QbpdzR2ywpxMIkmc1IEakKFxZ5oBxEklfrrX%2BkZUdROF0Gbo%2BkOSX1j3P8UxsohBavOBUcZ%2B1fbfhEhbMXPMTF9q%2FcFamQB8CeVtDe7%2FfhrJewQlHimtBNTUGIPci2xUKqbDrggWw6Sj5jwU0grw1g1lY3RPJdEgnfnHyw3%2BE%2BIbdrUSwOWwvZmwqdw%2FRsrKZqLkEKL6TGzosTs0L6s%2FA5%2FDd6%2BdlMVPhfcHx3GqIW4DdhmJfYG5O4H050JsvOaxDZh4OSHesc0p3zCUr6vPBjqZAZubfzhAOensKJwMmuw0QXUkLJcHHmCx0v7fEWgaOs6R8%2BI%2BHmZlpZa8ECKQKfS5XHSC81TAm01qj0i1kz9DHj7Cirz7ANfItit5ABz6ZhuBS7lqEMJRRLE80HrB5bcVd280zIrzn6Uev6Au9f1T69qwf0p4Orrg7eiGyH5QJ5jWCHDS2ZhAqJsqX5FdncMpj960p878zQXVIw%3D%3D&Expires=1776999052)

## Gherkin scenarios

```gherkin
Feature: Secure lockbox authentication

  Background:
    Given the secure lockbox is powered on
    And the system is on the main screen

  Scenario: Fingerprint is found in the R503 database but does not match any registered user
    When the user selects "Begin authentication"
    And the user selects "Fingerprint Scan"
    And the user selects "Begin Scan"
    And the user places a previously scanned finger on the fingerprint scanner
    Then the fingerprint scanner LED blinks blue 3 times
    And the fingerprint scanner LED blinks green 3 times
    And the system displays "No match (ok)"
    When the user selects "Back"
    Then the system returns to the biometric selection screen
    And the system displays "Fingerprint found: does not match any user"

  Scenario: Fingerprint is not found in the R503 database
    When the user selects "Begin authentication"
    And the user selects "Fingerprint Scan"
    And the user selects "Begin Scan"
    And the user places an unregistered finger on the fingerprint scanner
    Then the fingerprint scanner LED blinks blue 3 times
    And the fingerprint scanner LED blinks red 3 times
    And the system displays "No match (no match)"
    When the user selects "Back"
    Then the system returns to the biometric selection screen
    And the system displays "Fingerprint not found"

  Scenario: Face is found in the face database, matches a registered user, and the PIN is correct
    When the user selects "Begin authentication"
    And the user selects "Face Scan"
    And the user positions their face on the face recognition screen
    And the user selects "Begin Scan"
    Then the system proceeds to the PIN entry screen
    When the user enters PIN "4321"
    And the user selects "Submit"
    Then the system displays "PIN Accepted Unlocked"
    And the system proceeds to the home screen
    And the system displays "Welcome, Alice"
    And the lock status is "[LOCKED]"
    When the user selects "Unlock Secure Box"
    Then the system displays "Box Unlocked"
    And the lock status changes to "[UNLOCKED]"
```

## Suggestions for missing Gherkin details

A few items would make these tests more complete and easier to automate:

- Add a clearer **rule** for the authentication flow, such as: biometric success must be followed by PIN validation before unlock is allowed.
- Replace UI-specific wording like `goto:` and `observe:` with consistent Gherkin keywords only, which makes the scenarios more readable and executable.
- Define the exact expected text for messages when possible. For example, in the fingerprint-failure test you currently have two possible expected messages; Gherkin works better with one asserted outcome per scenario.
- Clarify whether `"Back"` should return to the biometric selection screen from both fingerprint and face flows.
- Add user/account setup in the `Background` or a `Given` step, for example:
  - registered user `Alice`
  - PIN `4321`
  - enrolled face and fingerprint data
- Clarify timing requirements as explicit expectations, such as:
  - "Then the success message remains visible for at least 2 seconds"
  - "And the screen remains on the result page before navigating"
- In the face-success test, the current script appears to jump to the home screen before showing the face-match message. That should be split into two separate expected outcomes if both are intended.
- Consider separating `Then` assertions from `When` actions more strictly, especially where the script currently mixes observation and navigation.

## One inconsistency to fix

Your written requirement says the user must perform **either fingerprint or face recognition AND then enter a correct PIN** before unlocking the box. The tested face scenario currently shows a second press of `"Unlock Secure Box"` after a valid PIN, which may be fine if that is the intended final action, but it is not clearly described in the rule. If the PIN alone should unlock the box, that step should be removed; if a final unlock button is required, the rule should say so explicitly. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/a9c00232-12e7-4453-aada-d818f8bd42ea/testing.md?AWSAccessKeyId=ASIA2F3EMEYEYM47R5KJ&Signature=hqaiIHamn2xjO8imREg6oV8NimQ%3D&x-amz-security-token=IQoJb3JpZ2luX2VjEKv%2F%2F%2F%2F%2F%2F%2F%2F%2F%2FwEaCXVzLWVhc3QtMSJGMEQCIAeN0Un2%2FoNn0pntXjOvv0N18zF7h2wn05bTbLc26TCyAiBh4S0onJJ8s3B0MjCrWdW9Q66KxpaCQWX%2FYFpL%2FvamhyrzBAh0EAEaDDY5OTc1MzMwOTcwNSIMrEbTFrUSVFeSyIjrKtAECyaZMZ4K9xrt6aHtkgTYCX6WfKKS7OaEQk8Tnk790ozoLYd9ZLGGSCgyDivj28dYHE8qb1uU6HbcYcD6PavOMUxAiaK8pITADt%2BAJmeR6G6bPoPZqHIEeWtQJR6cBUTSF2teYAluXnQ4aW789F3LOKHMmdUSpAfspVuwObtm4SR%2FWTtQxgINFSe7D6CMzhBgHbEB2iAMY2w5LyLLCp%2BUHxxWQXGGU4tSGYzDgpva9kPRyJPUcjxq%2B%2BKJfFUmprSFc0BALTfwpUPLYc7OfV%2F37tcnpUDwSdU9Vac59QMvA4bbDD61ZBKyvp8YneF3ASs16DkIoNFbu%2Fcs1uUUM9Vm0nlC88EMABzFKwEC%2FqfqTSGsL0TXzx3VDi9k%2B6gSeWbEDGwH0GIm%2BThM78aNvSWSxL7L0MgVTUooW5olCRqIFpyN04qLQN%2FI%2Bxa%2FgwbdoVcfbvgNlJhg4Tho%2BJF258SKJSCU2%2BAPrLjabH5RaFBO0JgHE8cZA74PeIbiEao4pq1xDVfBiBl1QbpdzR2ywpxMIkmc1IEakKFxZ5oBxEklfrrX%2BkZUdROF0Gbo%2BkOSX1j3P8UxsohBavOBUcZ%2B1fbfhEhbMXPMTF9q%2FcFamQB8CeVtDe7%2FfhrJewQlHimtBNTUGIPci2xUKqbDrggWw6Sj5jwU0grw1g1lY3RPJdEgnfnHyw3%2BE%2BIbdrUSwOWwvZmwqdw%2FRsrKZqLkEKL6TGzosTs0L6s%2FA5%2FDd6%2BdlMVPhfcHx3GqIW4DdhmJfYG5O4H050JsvOaxDZh4OSHesc0p3zCUr6vPBjqZAZubfzhAOensKJwMmuw0QXUkLJcHHmCx0v7fEWgaOs6R8%2BI%2BHmZlpZa8ECKQKfS5XHSC81TAm01qj0i1kz9DHj7Cirz7ANfItit5ABz6ZhuBS7lqEMJRRLE80HrB5bcVd280zIrzn6Uev6Au9f1T69qwf0p4Orrg7eiGyH5QJ5jWCHDS2ZhAqJsqX5FdncMpj960p878zQXVIw%3D%3D&Expires=1776999052)

A good next step would be to convert the two untested scenarios into the same Gherkin style so the whole feature reads consistently.

The best practice is to keep the Gherkin scenario focused on the **expected behavior**, and record failures in a separate execution note, defect entry, or report field rather than rewriting the scenario itself. For manual testing, each failed step should be documented with the **actual result**, the step where divergence occurred, and any evidence such as a screenshot or log note. [virtuosoqa](https://www.virtuosoqa.com/post/write-test-cases-in-manual-testing)

## Practical format to use

Use a structure like this for each execution:

- Scenario name.
- Preconditions.
- Step number and action.
- Expected result.
- Actual result.
- Status: Pass or Fail.
- Defect ID or issue link, if failed.
- Evidence: screenshot, log snippet, or video reference.

That approach matches common manual test documentation practices, where discrepancies are recorded as expected vs. actual results and failed cases are tied to a defect record. It also keeps the Gherkin readable and reusable, instead of turning it into a failure report. [club.ministryoftesting](https://club.ministryoftesting.com/t/how-do-you-document-your-functional-test-cases/21479)

## What to write when a step fails

For the failed step, write:

- What you did.
- What you expected.
- What actually happened.
- Whether the test stopped there or continued.
- What evidence you captured.

Example:

- Step 4: Enter PIN `4321`.
- Expected: System accepts PIN and proceeds to unlock flow.
- Actual: System displayed `PIN invalid`.
- Result: Fail.
- Evidence: Screenshot 2026-04-23-2215.png.
- Defect: BUG-142.

This style makes it easy for someone else to reproduce the failure and see exactly where the script stopped. [browserstack](https://www.browserstack.com/guide/take-screenshot-for-failed-test-cases-in-cucumber)

## Best way to highlight the mismatch

If you are using a test script manually, the clearest method is to mark the failing step inline and then add a short failure note immediately below it. For example:

- `Step 4 FAILED`
- `Actual: ...`
- `Expected: ...`

If your reporting tool supports it, add a dedicated `Failure:` note or equivalent field so the failure stands out in summaries. If screenshots are available, attach one for the exact screen at the moment of failure because that speeds up diagnosis. [johnfergusonsmart](https://johnfergusonsmart.com/reporting-manual-test-results-in-serenity-bdd/)

## Recommended rule of thumb

Keep the Gherkin file as the **specification of behavior**, and keep the execution log as the **record of what happened**. The scenario should say what should happen; the test run record should say what did happen. This separation makes the tests easier to maintain and makes failures much easier to review. [automationpanda](https://automationpanda.com/2017/01/30/bdd-101-writing-good-gherkin/)

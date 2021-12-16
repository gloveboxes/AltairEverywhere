#include "dx_pwm.h"

bool dx_pwmSetDutyCycle(DX_PWM_BINDING *pwmBinding, uint32_t hertz, uint32_t dutyCyclePercentage)
{
    if (pwmBinding->pwmController->initialized && IN_RANGE(dutyCyclePercentage, 0, 100) && IN_RANGE(hertz, 0, 100000)) {
        PwmState state = {.enabled = true, .polarity = pwmBinding->pwmPolarity};

        state.period_nsec = 1000000000 / hertz;
        state.dutyCycle_nsec = state.period_nsec * dutyCyclePercentage / 100;

        if (PWM_Apply(pwmBinding->pwmController->fd, pwmBinding->channelId, &state) == -1) {
            Log_Debug("PWM Channel ID (%d), aka (%s) apply failed\n", pwmBinding->channelId, pwmBinding->name);
            return false;
        }
        return true;
    }
    return false;
}

bool dx_pwmStop(DX_PWM_BINDING *pwmBinding)
{
    if (pwmBinding->pwmController->initialized) {
        // set arbitrary period and duraction. The important thing is enabled = false
        PwmState state = {.period_nsec = 100000, .dutyCycle_nsec = 100, .polarity = pwmBinding->pwmPolarity, .enabled = false};

        if (PWM_Apply(pwmBinding->pwmController->fd, pwmBinding->channelId, &state) == -1) {
            Log_Debug("PWM Channel ID (%d), aka (%s) apply failed\n", pwmBinding->channelId, pwmBinding->name);
            return false;
        }
        return true;
    }
    return false;
}

bool dx_pwmOpen(DX_PWM_BINDING *pwmBinding)
{
    if (pwmBinding->pwmController->initialized) {
        return true;
    }

    if ((pwmBinding->pwmController->fd = PWM_Open(pwmBinding->pwmController->controllerId)) == -1) {
        return false;
    } else {
        pwmBinding->pwmController->initialized = true;
    }

    return true;
}

void dx_pwmSetOpen(DX_PWM_BINDING **pwmSet, size_t pwmSetCount)
{
    for (int i = 0; i < pwmSetCount; i++) {
        if (!dx_pwmOpen(pwmSet[i])) {
            break;
        }
    }
}

bool dx_pwmClose(DX_PWM_BINDING *pwmBinding)
{
    if (pwmBinding->pwmController->initialized) {

        pwmBinding->pwmController->initialized = false;

        if (close(pwmBinding->pwmController->fd) != 0) {
            return false;
        }
    }

    return true;
}

void dx_pwmSetClose(DX_PWM_BINDING **pwmSet, size_t pwmSetCount)
{
    for (int i = 0; i < pwmSetCount; i++) {
        dx_pwmClose(pwmSet[i]);
    }
}
/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <applibs/application.h>
#include <applibs/log.h>
#include <applibs/networking.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#define ONE_MS 1000000
#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))
#define IN_RANGE(number, low, high) (low <= number && high >= number)
#define NULL_OR_EMPTY(string) (string == NULL || strlen(string) == 0)

bool dx_isDeviceAuthReady(void);
bool dx_isNetworkConnected(const char *networkInterface);
bool dx_isNetworkReady(void);
bool dx_isStringNullOrEmpty(const char *string);

/// <summary>
/// check string contain only printable characters
/// ! " # $ % & ' ( ) * + , - . / 0 1 2 3 4 5 6 7 8 9 : ; < = > ? @ A B C D E F G H I J K L M N O P Q
/// R S T U V W X Y Z [ \ ] ^ _ ` a b c d e f g h i j k l m n o p q r s t u v w x y z { | } ~
/// </summary>
/// <param name="data"></param>
/// <returns></returns>
bool dx_isStringPrintable(char *data);

bool dx_startThreadDetached(void *(daemon)(void *), void *arg, char *daemon_name);
char *dx_getCurrentUtc(char *buffer, size_t bufferSize);
int dx_stringEndsWith(const char *str, const char *suffix);
int64_t dx_getNowMilliseconds(void);
void dx_Log_Debug(char *fmt, ...);
void dx_Log_Debug_Init(const char *buffer, size_t buffer_size);
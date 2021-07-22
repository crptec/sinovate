# Руководство по установке Infinity Node

  

!> Пожалуйста используйте только **Ubuntu 18.04**

  

## I. Предварительная подготовка

  

* [x] Скачайте и установите кошелёк Sinovate из официального канала [Official Github channel].(https://github.com/SINOVATEblockchain/SIN-core/releases/latest)

* [x] 	Если у вас нет монет , купите их на биржах: [TradeOgre](https://tradeogre.com/exchange/BTC-SIN), [Stex.com](https://app.stex.com/en/basic-trade/pair/BTC/SIN/1D), [Crex24](https://crex24.com/exchange/SIN-BTC), [txbit.io](https://txbit.io/Trade/SIN/BTC/?r=c73), [Coinsbit](https://coinsbit.io/trade/SIN_BTC), [Catex](https://www.catex.io/trading/SIN/ETH).

* [x] Отправьте монеты себе на кошелёк.

* [x] Для запуска Infinity Node, вам понадобится 2 транзакции “BURN” и  “Collateral“.

* [x] Включите контроль входов в кошельке. Настройки ->Опции->Кошелёк-> Enable coin control features.

* [x] Создайте второй SIN адрес в другом кошельке,для того чтобы указать его как резервный адрес при создании Infinity Node .

  

**Infinity Nodes бывают трех видов:

  

| Tier | Burn | Collateral |
| :--- | :---: | :---: |
| SIN-MINI | 100,000 SIN | 10,000 SIN |
| SIN-MID | 500,000 SIN | 10,000 SIN |
| SIN-BIG | 1,000,000 SIN | 10,000 SIN |


  

Количество “Burn” представляет собой сумму монет, которые будут «сожжены» и больше не будут доступны для торговли.

  

Collateral - это залог (10 000 SIN) который останется заблокированным внутри вашего кошелька. Разблокировка и перемещение этих монет отключит узел
  

## II. НАЧАЛО УСТАНОВКИ

  

?> Когда вы открываете кошелек, **ВСЕГДА** дайте ему полностью синхронизироваться. Первый раз, когда вы его откроете, потребуется много времени, поэтому, пожалуйста, наберитесь терпения.

  

### 1. Создание резервного адреса SIN

  

В качестве дополнительной защиты, новая функция была включена с Infinity Node, это **резервный адрес**. Если по какой-либо причине ваш компьютер будет взломан, а ваш локальный кошелек скомпрометирован, создание нового узла Infinity с резервным адресом из другого источника кошелька позволит избежать его потерю.

  

Следуйте этим шагам для создания вашего резервного SIN адреса.

  

* Откройте [https://sinovate.io/sin-wallets/](https://sinovate.io/sin-wallets/)

* Скачайте кошелек для [Android](https://play.google.com/store/apps/details?id=io.sinovate.wallet) или [Apple IOS](https://apps.apple.com/gb/app/sinovate-wallet/id1483969772) .

* В поле receive в вашем мобильном кошельке, нажмите `COPY`.

![Image-bkup-001](assets/img/infinity_node_setup_guide/mobilcopy.png)

  

* :warning: **Теперь это ваш резервный адрес ** Смотрите скриншот ниже:

  

![Image-bkup-002](assets/img/infinity_node_setup_guide/mobilcopied.png)

  
  

* Отправьте **РЕЗЕРВНЫЙ** адрес себе на PC со своего Мобильного используя удобный вам метод.

  

* :warning: :key: _** ***Никогда не при каких обстоятельствах не делитесь приватными ключами с другими людьми***.**_ :warning: :key:

* После того как отправили резервный адрес себе на PC, сохраните его.И переходите к своему полностью синхронизированному локальному кошельку.

  
  

### 2. Транзакция сжигания "Burn"

  

Откройте свой локальный кошелек Sinovate и создайте новый адрес получения:

  

* В верхнем меню выберите Файл, затем Адреса для получения.

* Дайте метку адресу (для примера: 01-MINI – как на скриншоте).

  

![Image 01](assets/img/infinity_node_setup_guide/receiving.png)

![Image 005](assets/img/infinity_node_setup_guide/01-MINI.png)

  
  

* Скопируйте этот адрес в буфер обмена.

* Перейдите на вкладку **SEND** кошелька и вставьте адрес в поле Pay To.

![Image 02](assets/img/infinity_node_setup_guide/send.png)

* В поле Amount введите ту сумму в зависимости от того какую Infinity Node вы хотите запустить. (100,000 / 500,000 / 1,000,000) –смотрите на скриншоте ниже.

* Cумма должна быть точной, не больше, не меньше.

  

![Image 009](assets/img/infinity_node_setup_guide/send100k.png)

  
  

:warning:**ВАЖНО:** НЕ ставьте маленькую галочку с надписью "Subtract fee from amount”. Оставьте как есть.

  

* Перейдите на вкладку  **HISTORY** и подождите, пока транзакция получит **2 подтверждения**.

* После получения подтверждений вернитесь на вкладку  **SEND** и нажмите на  «Контроль входов»

* Список с балансами должен открыться. Выберите сумму с суммой BURN (в нашем примере это будет 100 000 монет). Вы выбираете его, установив маленький флажок слева и нажимая кнопку «ОК» для подтверждения. Это обеспечит выполнение следующего процесса, транзакции сжигания, только из этого источника.

  

![Image 03](assets/img/infinity_node_setup_guide/coincontrol.png)

  

* В верхнем меню кошелька нажмите Настройки ->Помощь , затем Debug Window (Console).

* Прежде чем вводить команду сжигания, обязательно разблокируйте кошелек, если ваш кошелек был зашифрован. откройте Debug Window(Console) и введите команду: walletpassphrase password 999 (замените password вашим паролем). 999 это количество секунд на которые ваш кошелек будет разблокирован.

* Как показанно на скриншоте ниже, введите команду сжигания в консоли. Командой будет  **infinitynodeburnfund**,затем  ваш адрес, количество в зависимости от ноды (100000 / 500000 / 1000000) и ваш резервный адрес.

* Пожалуйста, используйте резервный адрес, ранее сгенерированный в вашем мобильном кошельке. \(Секция 1. Создание резервного адреса SIN\)

* Резервный адрес не должен быть из этого же wallet.dat чтобы в случае кражи хакер не завладел ими обоими._

* **	Убедитесь, что команда введена правильно, потому что вы больше не сможете восстановить эти монеты.**

* На скриншоте ниже пример для SIN-MINI Infinity Node, поэтому команда в этом случае
  

```

infinitynodeburnfund yourSINaddress 100000 yourSINbackupaddress

```

  

![Image 04](assets/img/infinity_node_setup_guide/console.png)

  

!> **ПОМНИТЕ: СКРИНШОТ СЛУЖИТ ТОЛЬКО ДЛЯ ПРИМЕРА! Если у вас есть какие-либо сомнения на данный момент, лучше всего обратиться в службу поддержки Sinovate, прежде чем вводить команду, без полного понимания последствий ошибки.**

  

* После того, как вы ввели команду BURN, вы получите вывод, похожий на тот, что на скриншоте ниже.

  

![Image 05](assets/img/infinity_node_setup_guide/burnfunds.png)

  

* На вкладке **HISTORY**  получите информацию о транзакции **Burn**, дважды щелкнув по транзакции. Скопируйте **Transaction ID** и **Output Index** в Блокнот, эта информация понадобится вам позже.

  

![Image 06](assets/img/infinity_node_setup_guide/transaction.png)

  

### 3. Транзакция залога "Collateral" 

  

В отличие от транзакции «Burn», в результате которой монеты оказываются вне досягаемости, что делает их не подлежащими возврату, залоговая транзакция **Collateral** содержит 10 000 монет SIN, которые останутся заблокированными в вашем локальном кошельке, но полностью под вашим контролем.


  

* Перейдите во вкладку Send,в вашем кошельке Sinovate , и отправьте ровно 10,000 SIN на ТОТ ЖЕ АДРЕС что вы использовали для транзакции сжигания (BURN) .

* Во вкладке **HISTORY**, получите информацию  о транзакции залога нажав на неё 2 раза . Скопируйте **Transaction ID** и **Output Index** в Блокнот,эта информация понадобится вам позже. 

  

![Image 07](assets/img/infinity_node_setup_guide/collateral.png)

  

### 4. Создание приватного ключа для Infinity Node 

  

* ⦁	Снова откройте консоль. Наберите следующую команду для генерации нового masternode privkey: **masternode genkey**. **Скопируйте приватный ключ в блокнот он нам дальше понадобиться**. 

  

![Image 08](assets/img/infinity_node_setup_guide/genkey.png)

  

### 5. Редактирование файла infinitynode.conf 

  

* 	В кошельке выберите настройки ->Tools затем Open Infinitynode Configuration File

* ⦁	В новой строке введите строку конфигурации infinitynode, как указано в следующих шагах, получая всю необходимую информацию из **Блокнота** в который вы ранее эту информацию сохраняли. 
  

**Строка состоит из:**

  

| Alias | VPS IP:PORT | privkey | Collateral tx ID | Collateral Output index | Burn tx ID | Burn Output index |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: |


  
  

**Должна выглядеть как в следующем примере:**

  

| Alias | VPS IP:PORT | privkey | Collateral tx ID | Collateral Output index | Burn tx ID | Burn Output index |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| 01-MINI | 78.47.162.140:20970 | 7RRfQkxYPUKkKFAQBpoMde1qaB56EvPU5X8LYWq16e2YtTycvVi | 7f48e48e51b487f0a962d492b03debdd89835bc619242be29e720080fc4b2e09 | 0 | 764fe088b95d287b56f85ee0da11bb08195a862ded8b7ded08a3783135418e3c | 0 |


  

**Скриншот файла infinitynode.conf :**

  

![Image 09](assets/img/infinity_node_setup_guide/img_09.jpg)

  

* Сохраните файл, затем **перезапустите кошелек**.

  

## III. Настройка VPS

### Шаг 1

  

Войдите или создайте аккаунт на [setup.sinovate.io](https://setup.sinovate.io/).

  

[![](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image15.png)](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image15.png)

  

### Шаг 2

  

При входе в систему нажмите вкладку **SERVICES** и выберите **ORDER NEW SERVICES** в раскрывающемся меню.

  

[![](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image14.png)](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image14.png)

  
  

### Шаг 3

  

Выберите свой биллинговый цикл, 12 месяцев, конечно, гораздо выгоднее. После того, как вы выбрали свой платежный цикл, нажмите **CONTINUE**.

  

[![](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image6.png)](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image6.png)

  

### Шаг 4

  

Нажмите **CHECKOUT**, если вы довольны своим заказом.

  

[![](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image4.png)](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image4.png)

  
  

### Шаг 5

  

Введите свой адрес электронной почты, а также свой Discord, если хотите, а затем нажмите **COMPLETE ORDER**.

  

[![](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image2.png)](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image2.png)

  

### Шаг 6

  

ОПЛАТИТЕ необходимое количество SIN на указанный адрес. Пожалуйста, обратите внимание, **НЕ ЗАКРЫВАЙТЕ ЭТУ СТРАНИЦУ**. 

  

[![](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image10.png)](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image10.png)

  
  

### Шаг 7

  

Отправьте нужную **СУММУ SIN-монет** на адрес, который вам указали в счете на шаге 6.


  

[![](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image11a.png)](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image11a.png)

  

### Шаг 8

  

После этого вы будете отправлены на страницу ORDER CONFIRMATION. **Обратите внимание, что для обработки этой страницы может потребоваться 15 минут**. 

  

[![](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image8.png)](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image8.png)

  
  

### Шаг 9

  

Нажмите на  **SERVICES**.

  

[![](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image17.png)](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image17.png)

  

### Шаг 10

  

Нажмите на секции  **INFINITY NODE HOSTING**. 

  

[![](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image12.png)](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image12.png)

  
  
  

### Шаг 11

  

Дождитесь окончательного подтверждения того, что ваш **SERVER** готов.

  

[![](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image16.png)](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image16.png)

  

### Шаг 12

  

Как только ваш сервер будет готов, вам будет предоставлено **ДВЕ ВАЖНЫХ ЗАПИСИ**, которые вам понадобятся для редактирования **INFINITY CONFIGURATION FILE** вашего  infinity node. Они будут обведены красным, смотрите скриншот ниже.
  

[![](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image1.png)](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image1.png)

  
  
  

### Шаг 13

  

Нажмите **настройки->Tools** и затем **OPEN INFINITY CONFIGURATION FILE**.

  

[![](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image13.png)](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image13.png)

  

### Шаг 14

  

Из **информации**, полученной на шаге 12,поместите записи в  **INFINITY NODE CONFIGURATION FILE** как показано ниже. Сохраните и закройте файл и кошелек.

  

[![](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image7.png)](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image7.png)

  
  
  

### Шаг 15

  

Откройте кошелек и подождите пока он полностью синхронизируется.

Выберите вкладку INFINITY NODE. 

Нажмите START ALIAS.

Нажмите YES.

_Ваш INFINITY NODE перейдет в режим **PRE-ENABLED** и через некоторое время в режим **ENABLED**_

  

[![](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image5.png)](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image5.png)

  

### Шаг 16

  

Поздравляем вы **успешно запустили** свою **INFINITY NODE**.

  

[Start](https://setup.sinovate.io/getcycle.php)

  

[![](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image9.png)](https://setup.sinovate.io/templates/sinsetup/template/img/logos/images/image9.png)

  
  
  
  
  



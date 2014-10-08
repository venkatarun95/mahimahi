/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <string>

#ifndef SELENIUM_CONFIGURATION_HH
#define SELENIUM_CONFIGURATION_HH

std::string selenium_setup = "\nprint site\nimport sys\nfrom selenium import webdriver\nfrom selenium.webdriver.chrome.options import Options\nfrom selenium.webdriver.common.action_chains import ActionChains\nfrom selenium.webdriver.common.keys import Keys\nfrom pyvirtualdisplay import Display\nimport os\nfrom time import sleep\ndisplay = Display(visible=0, size=(800,600))\ndisplay.start()\noptions=Options()\noptions.add_argument(\"--incognito\")\noptions.add_argument(\"--ignore-certificate-errors\")\nchromedriver = \"/home/ravi/chromedriver\"\ndriver=webdriver.Chrome(chromedriver, chrome_options=options)\ndriver.set_page_load_timeout(40)\ndriver.get(site)\ndriver.quit()\ndisplay.stop()";


#endif /* SELENIUM_CONFIGURATION_HH */

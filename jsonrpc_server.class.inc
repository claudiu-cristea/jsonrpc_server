<?php

/**
 * @file
 * JSON-RPC Class file.
 *
 * References:
 * - JSON-RPC 1.1 draft: http://json-rpc.org/wd/JSON-RPC-1-1-WD-20060807.html
 * - JSON-RPC 2.0 proposal: http://groups.google.com/group/json-rpc/web/json-rpc-1-2-proposal
 *
 * Error codes follows the json-rpc 2.0 proposal as no codes exists for the 1.1 draft.
 */

/**
 * -32700: Parse error. Invalid JSON. An error occurred on the server while parsing the JSON text.
 */
define('JSONRPC_ERROR_PARSE', -32700);

/**
 * -32600: Invalid Request. The received JSON not a valid JSON-RPC Request.
 */
define('JSONRPC_ERROR_REQUEST', -32600);

/**
 * -32601: Method not found. The requested remote-procedure does not exist / is not available.
 */
define('JSONRPC_ERROR_PROCEDURE_NOT_FOUND', -32601);

/**
 * -32602: Invalid params. Invalid method parameters.
 */
define('JSONRPC_ERROR_PARAMS', -32602);

/**
 * -32603: Internal error. Internal JSON-RPC error.
 */
define('JSONRPC_ERROR_INTERNAL_ERROR', -32603);

/**
 * -32099..-32000: Server error. Reserved for implementation-defined server-errors.
 */

/**
 * Represents a JSON-RPC Server instance.
 */
interface JsonRpcServerInterface {

  /**
   * Handle a JSON-RPC request and returns the response.
   *
   * @return
   * A response or throw an ServicesException.
   */
  public function handle();

  /**
   * Add additional informations to the response.
   *
   * The response must be according to JSON-RPC specifications. The main result
   * will be wrapped together with other information.
   *
   * @param array $response
   * An Associative array containing the basic response of the service.
   *
   * @return
   * JSON encoded string with the complete response for this request.
   */
  public function buildResponse($response);
}

/**
 * Handle a JSON-RPC request.
 *
 * Default implementation of JsonRpcServerInterface.
 */
class JsonRpcServer implements JsonRpcServerInterface {

  /**
   * The JSON-RPC Request identifier.
   *
   * @var JSON scalar (String, Number, True, False)
   */
  private $id;

  /**
   * The JSON-RPC Request method name.
   *
   * @var String
   */
  private $methodName;

  /**
   * The Services method definition.
   *
   * @var Associative Array
   */
  private $method;

  /**
   * The decoded JSON-RPC Request.
   *
   * @var Associative Array
   */
  private $input;

  /**
   * The JSON-RPC HTTP Request method.
   *
   * @var String.
   */
  private $httpMethod;

  /**
   * The JSON-RPC Request version.
   *
   * @var String.
   */
  private $version;

  /**
   * The JSON-RPC Request major version.
   *
   * @var Integers
   */
  private $majorVersion;

  /**
   * The JSON-RPC Request parameters.
   *
   * @var Array or Object.
   */
  private $params;

  /**
   * Method defined arguments.
   *
   * @var Array
   */
  private $args;

  /**
   * Method expected arguments count.
   *
   * @var Integer
   */
  private $methodArgumentsCount;

  /**
   * If parameters were passed as an  passed paramete

  /**
   * Construct a JsonRpcServer object.
   *
   * @param array $input
   * The JSON decoded input.
   * @param string $http_method
   * The HTTP method of the request.
   *
   * The decoded JSON-RPC Request.
   */
  public function __construct($input, $http_method) {
    $this->input = $input;
    $this->httpMethod = $http_method;
    $this->methodName = isset($input['method']) ? $input['method'] : NULL;
    $this->id = isset($input['id']) ? $input['id'] : NULL;
    $this->version = isset($input['jsonrpc']) ? $input['jsonrpc'] : '1.1';
    $this->majorVersion = intval(substr($this->version, 0, 1));
    $this->params = isset($input['params']) ? $input['params'] : array();
    $this->args = array();
  }

  /**
   * Validate and prepare a JSON-RPC call according to specifications.
   *
   * Not all the validations a made here.
   */
  protected function validate() {

    // A method is required.
    if (empty($this->methodName)) {
      $this->error(JSONRPC_ERROR_REQUEST, t('The received JSON not a valid JSON-RPC Request.'));
    }

    // Check for if valid "params" were sent.
    if (!is_array($this->params)) {
      $this->error(JSONRPC_ERROR_PARSE, t('No valid parameters received. The "params" member of JSON must be an Array or Object.'));
    }

    $endpoint = services_get_server_info('endpoint');

    // Find the method definition as it was provided by the Services module.
    $this->method = services_controller_get($this->methodName, $endpoint);

    // Check id this method has been defined in hook_services_resources().
    if (!isset($this->method)) {
      $this->error(JSONRPC_ERROR_PROCEDURE_NOT_FOUND, t('Invalid method "@method".', array('@method' => $this->methodName)));
    }
    $this->method['args'] = isset($this->method['args']) ? $this->method['args'] : array();

    $count_params = count($this->params);
    $this->methodArgumentsCount = count($this->method['args']);

    // Too many parameters passed.
    if ($count_params > $this->methodArgumentsCount) {
      if ($this->methodArgumentsCount == 0) {
        $this->error(JSONRPC_ERROR_PARAMS, t('@params parameters were passed but this method (@method) accepts none.', array('@params' => $count_params, '@method' => $this->methodName)));
      }
      elseif ($this->methodArgumentsCount == 1) {
        $this->error(JSONRPC_ERROR_PARAMS, t('@params parameters were passed but this method (@method) accepts only one.', array('@params' => $count_params, '@method' => $this->methodName)));
      }
      else {
        $this->error(JSONRPC_ERROR_PARAMS, t('@params parameters were passed but this method (@method) accepts only @args.', array('@params' => $count_params, '@args' => $this->methodArgumentsCount, '@method' => $this->methodName)));
      }
    }

    // JSON-RPC 2.0 doesn't allow mixture of named and positional parameters. Let's check that.
    if ($this->majorVersion == 2) {
      foreach ($this->params as $param_key => $param_value) {

        // Get only the first param type.
        if (!isset($param_type)) {
          $param_type = is_numeric($param_key) ? 0 : 1;

          // Just iterate after the first step.
          continue;
        }

        // Found parameter mixture? Break with an error.
        if ((is_numeric($param_key) && $param_type == 1) || (is_string($param_key) && $param_type == 0)) {
          $this->error(JSONRPC_ERROR_PARAMS, t("JSON-RPC version @ver doesn't alow mixture of named and positional parameters.", array('@ver' => $this->version)));
        }
      }
    }

    // Build a list of argument names.
    $args = array();
    foreach ($this->method['args'] as $arg) {
      $args[] = $arg['name'];
    }

    foreach ($this->params as $param_key => $param_value) {
      // Check for positional parameters out-of-range.
      if (is_numeric($param_key) && ($param_key >= $this->methodArgumentsCount)) {
        $this->error(JSONRPC_ERROR_PARAMS, t('The index @position of the positional parameter @value is outside arguments range. Maximum allowed index @max.', array('@position' => $param_key, '@value' => drupal_json_encode($param_value), '@max' => ($this->methodArgumentsCount - 1))));
      }
      // And for invalid names of named parameters.
      elseif (is_string($param_key) && !in_array($param_key, $args)) {
        $this->error(JSONRPC_ERROR_PARAMS, t('Unknown named parameter "@name" received.', array('@name' => $param_key)));
      }
    }
  }

  /**
   * Validate a single passed parameter against the method argument definition.
   *
   * @param Integer $delta
   * The position in the method argument list.
   * @param Array $arg
   * The method argument definition.
   * @param Arbitrary Variable $param
   */
  protected function validateParam($delta, array $arg, &$param) {

    // Check if the argument is mandatory but no parameter has been passed.
    if (!$arg['optional'] && !isset($param)) {
      $this->error(JSONRPC_ERROR_PARAMS, t('The argument "@arg" (index @delta) is required by the "@method" method but was not received.', array('@arg' => $arg['name'], '@delta' => $delta, '@method' => $this->methodName)));
    }

    /**
     * If "struct" is expected, convert associative array to object but only if
     * the associative array has no numeric (positional) keys. Forcing those to
     * object will remove them.
     *
     * Example: (object) array('somekey' => 'value', 2 => 'value2') results in
     * {"somekey": "value"}. The numeric key item get lost.
     */
    if (is_array($param) && ($arg['type'] == 'struct')) {
      if (!((bool) count(array_filter(array_keys($param), 'is_numeric')))) {
        $param = (object) $param;
      }

      // We are leaving here. The parameter is either on Object,
      // either an Array and a "struct" it's expected.
      return;
    }

    // Only array-type parameters accepts arrays.
    if (is_array($param) && $arg['type'] != 'array') {
      $this->errorWrongType($arg, 'array');
    }

    // Check that "int" or "float" value type arguments get numeric values.
    if (in_array($arg['type'], array('int', 'float')) && !is_numeric($param)) {
      $this->errorWrongType($arg, 'string');
    }
  }

  /**
   * Handle a JSON-RPC request and returns the response.
   *
   * @return
   * A response or throw an ServicesException.
   */
  public function handle() {

    // Validate & prepare.
    $this->validate();

    // Processing all sent parameters in the method argument order.
    foreach ($this->method['args'] as $delta => $arg) {

      /**
        * Check if there are 2 candidate parameters for the same argument.
        * This may happen only in JSON-RPC 1.1 where a positional paramater may
        * overlap a named parameter or viceversa.
        */
      if (($this->majorVersion == 1) && isset($this->params[$delta]) && isset($this->params[$arg['name']])) {
        $this->error(JSONRPC_ERROR_PARAMS, t('Two parameters "@delta": @positional and "@name": @named are disputing the same argument "@name".', array('@delta' => $delta, '@positional' => drupal_json_encode($this->params[$delta]), '@name' => $arg['name'], '@named' => drupal_json_encode($this->params[$arg['name']]))));
      }

      // Find out the parameter for this argument.
      $param = isset($this->params[$arg['name']]) ? $this->params[$arg['name']] : (isset($this->params[$delta]) ? $this->params[$delta] : NULL);

      if (!is_null($param)) {
        // Parameter level validation.
        $this->validateParam($delta, $arg, $param);
      }

      // Optional argument.
      if ($arg['optional']) {

        // No parameter sent for this argument.
        if (is_null($param)) {
          $this->args[] = isset($arg['default value']) ? $arg['default value'] : NULL;
        }
        // It's optional but a parameter has been sent. Use it.
        else {
          $this->args[] = $param;
        }
      }
      // Mandatory argument.
      else {
        $this->args[] = $param;
      }
    }

    // Call the service method.
    try {

      // Using the Services controller.
      $result = services_controller_execute($this->method, $this->args);
      return $this->result($result);
    }

    // On error, expect a ServicesException first.
    catch (ServicesException $e) {

      // Check first if the application has sent an error code. If yes, use it.
      $application_error_code = $e->getCode();
      $error_code = empty($application_error_code) ? JSONRPC_ERROR_INTERNAL_ERROR : $application_error_code;

      $this->error($error_code, $e->getMessage(), $e->getData());
    }

    // Fallback to a common Exception.
    catch (Exception $e) {
      $this->error(JSONRPC_ERROR_INTERNAL_ERROR, $e->getMessage());
    }
  }

  /**
   * Respond to the JSON_RPC Request.
   *
   * @param Arbitrary Variable $result.
   *
   * @return String
   * JSON encoded string with the service response.
   */
  protected function result($result) {
    $response = array('result' => $result);

    // Inform the browser that we are returning JSON.
    drupal_add_http_header('Content-Type', 'application/json; charset=utf-8');

    // Add other JSON-RPC protocol informations to the response and sent it.
    return $this->buildResponse($response);
  }

  /**
   * Add additional informations to the response.
   *
   * The response must be according to JSON-RPC specifications. The main result
   * will be wrapped together with other information.
   *
   * @param array $response
   * An Associative array containing the basic response of the service.
   *
   * @return
   * JSON encoded string with the complete response for this request.
   */
  public function buildResponse($response) {

    /*
     * In JSON-RPC 2.0 requests made without sending the Request ID ("id") are
     * notifications. So we are still let the consumer that his request is OK by
     * sending 200 but will not send any response.
     *
     * From version 2.0 specs:
     *
     * "A Notification is a special Request, without "id" and without Response.
     * The server MUST NOT reply to a Notification."
     *
     * @todo Such request must receive a 204 HTTP Code.
     */
    if ($this->majorVersion == 2 && empty($this->id)) {
      return;
    }

    // Add the version to the response.
    $this->responseAddVersion($response);

    // Add the Request ID ("id") to the response.
    $this->responseAddId($response);

    // Encode the response.
    return drupal_json_encode($response);
  }

  /**
   * Add JSON-RPC Version to a response.
   *
   * @param array $response
   * An Associative array containing the basic response of the service.
   */
  protected function responseAddVersion(&$response) {
    if ($this->majorVersion == 1) {
      $response['version'] = '1.1';
    }
    elseif ($this->majorVersion == 1) {
      $response['jsonrpc'] = '2.0';
    }
  }

  /**
   * Add the Request ID ("id") to a response.
   *
   * @param array $response
   * An Associative array containing the basic response of the service.
   */
  protected function responseAddId(&$response) {
    if (!empty($this->id)) {
      $response['id'] = $this->id;
    }
  }

  /**
   * General error processing function.
   *
   * @param Integer $code
   * The error code.
   * @param String $message
   * The error message.
   * @param Array $data
   * An associative array with additional data to be passed.
   */
  protected function error($code, $message, $data = NULL) {
    $response = array('error' => array('name' => 'JSONRPCError', 'code' => $code, 'message' => $message));
    if ($data) {
      $response['data'] = $data;
    }
    throw new ServicesException($message, $code, $response);
  }

  /**
   * Throw an wrong type error.
   *
   * @param Array $arg
   * The method argument definition array.
   * @param String $type
   * The variable type to which this error is referring.
   */
  protected function errorWrongType(array $arg, $type) {
    $this->error(JSONRPC_ERROR_PARAMS, t('The argument "@arg" should have "@type" type, not "@used_type".', array('@arg' => $arg['name'], '@type' => $arg['type'], '@used_type' => $type)));
  }
}

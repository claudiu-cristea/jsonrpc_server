<?php

/**
 * @file
 * Settings form definitions for JSON RPC Server.
 */

/**
 * JSON-RPC Server settings form.
 *
 * Generates the form fragment for configuring the JSON-RPC server
 * for an endpoint.
 *
 * @param array $form
 *  The form fragment from services that we should add our fields to.
 * @param object $endpoint
 *  The endpoint that we're configuring the REST server for.
 * @param array $settings
 *  The current settings.
 *
 * @return void
 */
function _jsonrpc_server_settings(&$form, $endpoint, $settings) {
  $form['allow_get_requests'] = array(
    '#type' => 'checkbox',
    '#title' => t('Allow GET requests'),
    '#description' => t('Allow consumers to send GET requests against this endpoint.'),
    '#default_value' => isset($settings['allow_get_requests']) ? $settings['allow_get_requests'] : FALSE,
  );
}

/**
 * Submit handler for the services JSON-RPC Server settings form.
 *
 * @param object $endpoint
 *  The endpoint that's being configured.
 * @param array $values
 *  The partial form-state from services.
 *
 * @return array
 *  The settings for the REST server in this endpoint.
 */
function _jsonrpc_server_settings_submit($endpoint, &$values) {
  return $values;
}
